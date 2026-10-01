#include "shadow/boot/InitialBootEvents.hpp"

#include <algorithm>
#include <stdexcept>

namespace shadow::boot {
namespace {
constexpr std::int32_t MaxSlice = 20000;
constexpr std::uint32_t UnitFactorBits = 0x3f800000u;
constexpr std::uint32_t ResetCause = 0x10000u, ViCause = 0x100u, ExternalException = 0x4u;
constexpr std::uint64_t DtkUserdata = (std::uint64_t{3} << 32u) | 1u;
void Require(bool valid, const char* message) {
    if (!valid) throw std::invalid_argument(message);
}
std::uint64_t Raw(std::int64_t value) { return static_cast<std::uint64_t>(value); }
}
bool InitialEventBefore(const InitialBootEvent& a, const InitialBootEvent& b) {
    return a.deadline < b.deadline || (a.deadline == b.deadline && a.fifo < b.fifo);
}
std::uint32_t ProjectInitialPiException(std::uint32_t cause, std::uint32_t mask,
                                        std::uint32_t exceptions) {
    return (cause & mask) ? exceptions | ExternalException : exceptions & ~ExternalException;
}
const char* InitialEventName(InitialEventKind kind) {
    switch (kind) {
    case InitialEventKind::Dtk: return "FinishExecutingCommand";
    case InitialEventKind::Gpu: return "GPUSleeper";
    case InitialEventKind::Vi: return "VICallback";
    case InitialEventKind::Dsp: return "DSPCallback";
    case InitialEventKind::Audio: return "AudioDMACallback";
    case InitialEventKind::Patch: return "PatchEngine";
    }
    throw std::invalid_argument("unknown initial event type");
}
const char* InitialBootStopName(InitialBootStop stop) {
    switch (stop) {
    case InitialBootStop::NotAdvanced: return "not-advanced";
    case InitialBootStop::Advancing: return "advancing";
    case InitialBootStop::DtkAudioLogRead: return "Mixer.cpp:253 live-dtk-log";
    case InitialBootStop::DtkAudioLogBranch: return "Mixer.cpp:255 unsupported-wave-writer";
    case InitialBootStop::GpuSleepEffect: return "BlockingLoop.h:233 undelivered-allow-sleep";
    case InitialBootStop::MovieFrameCounterRead: return "Movie.cpp:175 live-frame-counter";
    case InitialBootStop::NewFieldFrameStepRead: return "Core.cpp:880 live-frame-step";
    case InitialBootStop::NewFieldFrameStepBranch: return "Core.cpp:885 unsupported-frame-step";
    case InitialBootStop::AchievementClientRead: return "AchievementManager.cpp:242 live-client";
    case InitialBootStop::AchievementClientBranch: return "rc_client.c:3498 unsupported-client";
    case InitialBootStop::AchievementDllRead: return "AchievementManager.cpp:343 live-dll";
    case InitialBootStop::AchievementDllBranch: return "AchievementManager.cpp:347 unsupported-active-achievements";
    case InitialBootStop::FirstAdvanceComplete: return "first-advance-complete";
    }
    throw std::invalid_argument("unknown initial event stop");
}
InitialBootSourceConfig FreshInitialBootSourceConfig42() {
    InitialBootSourceConfig result;
    result.profile = InitialBootSourceConfig::Profile::FreshGcPalHle42;
    result.cpu_hz = 486000000u;
    result.scheduler_factor_bits = UnitFactorBits;
    result.vi_factor_bits = UnitFactorBits;
    return result;
}
void InitialBootEventOwner::Record(const char* kind, const char* name,
                                  std::array<std::uint64_t,8> values) {
    journal_.push_back({kind,name,values});
}
void InitialBootEventOwner::ClockRecord(const char* kind, std::uint64_t extra) {
    // PC belongs to the separately owned apploader module; no guessed PC.
    Record(kind,"-", {Raw(state_.global_timer),Raw(state_.slice_length),Raw(state_.downcount),
                       state_.sane ? 1u : 0u,state_.next_fifo,0u,
                       state_.inverse_factor_bits,extra});
}
void InitialBootEventOwner::Schedule(InitialEventKind kind, std::int64_t relative,
                                    std::uint64_t userdata) {
    Require(state_.sane,"scheduling outside finite advance/init scope");
    InitialBootEvent event{state_.global_timer+relative,state_.next_fifo++,kind,userdata};
    state_.queue.push_back(event);
    std::sort(state_.queue.begin(),state_.queue.end(),InitialEventBefore);
    Record("enqueue",InitialEventName(kind),{Raw(event.deadline),event.fifo,userdata,Raw(relative)});
}
InitialBootEventOwner::InitialBootEventOwner(const InitialBootSourceConfig& config,
                                           const InitialBootBranchBindings& branches)
    : config_(config), branches_(branches) {
    Require(config.profile==InitialBootSourceConfig::Profile::FreshGcPalHle42,
            "unknown source initialization profile");
    Require(config.cpu_hz==486000000u && config.scheduler_factor_bits==UnitFactorBits &&
            config.vi_factor_bits==UnitFactorBits,"unproved source clock/configuration");
    Require(!config.restored_state && !config.extra_initial_events,
            "restored/extra event initialization is unsupported");
    Require(branches.movie_lifecycle==InitialBootBranchBindings::MovieLifecycle::Unknown ||
            branches.movie_lifecycle==InitialBootBranchBindings::MovieLifecycle::FreshInactive,
            "unknown movie lifecycle enum");

    // Fresh last factor=0 produces downcount0 before RefreshConfig.
    state_.slice_length=MaxSlice; state_.sane=true;
    ClockRecord("init");
    state_.inverse_factor_bits=UnitFactorBits;
    ClockRecord("init-ready");

    // AI Init replaces constructor divisors with GC48KHz/32KHz.
    state_.ai.ais_divisor=1124u*2u;
    state_.ai.aid_divisor=state_.ai.ais_divisor*3u/2u;
    // VI Init/PAL boot preset; field timing is identical in both presets.
    auto& vi=state_.vi;
    vi.hlw=429u; vi.equalization=6u;
    vi.odd_prb=502u; vi.odd_psb=5u; vi.even_prb=503u; vi.even_psb=4u;
    vi.next_si_poll=15u;
    vi.odd_first=3u*vi.equalization+vi.odd_prb;
    vi.odd_last=vi.odd_first+2u*vi.active_lines-1u;
    vi.even_first=vi.odd_first+2u*vi.active_lines+vi.odd_psb+
        3u*vi.equalization+vi.even_prb-(vi.odd_psb-vi.even_psb);
    vi.even_last=vi.even_first+2u*vi.active_lines-1u;
    vi.interrupts[0]={430u,263u,true,false};
    vi.interrupts[1]={1u,1u,true,false};
    state_.pi.cause=ResetCause|ViCause;
    // DSP Reinit/HLE ROM initialize; ROM.Update has an empty body.
    state_.dsp.hle_rom=true; state_.dsp.mail_halted=true;
    state_.dsp.rom_mail=0x8071feedu;
    // Movie fields are unknown in the unbound reference capsule. This
    // explicit inactive control constructs them; it accepts no captured count.
    if (branches.movie_lifecycle==InitialBootBranchBindings::MovieLifecycle::FreshInactive) {
        state_.movie.frame=0u; state_.movie.lag=0u;
        state_.movie.total_frames=0u; state_.movie.total_lag=0u;
        state_.movie.polled=false;
    }
    // DVD ResetDrive(false) clears streaming/DMA and pending blocks, then
    // schedules its DTK/TCINT command before SystemTimers initialization.
    Schedule(InitialEventKind::Dtk,0,DtkUserdata);
    Schedule(InitialEventKind::Gpu,0);
    const auto half_line=(2u*config.cpu_hz/27000000u)*vi.hlw;
    Schedule(InitialEventKind::Vi,half_line);
    Schedule(InitialEventKind::Dsp,0);
    const auto audio_period=std::uint64_t(config.cpu_hz)*state_.ai.aid_divisor/
        (108000000u*4u/32u);
    Schedule(InitialEventKind::Audio,static_cast<std::int64_t>(audio_period));
    const auto even_half_lines=3u*vi.equalization+vi.even_prb+
        2u*vi.active_lines+vi.even_psb;
    Schedule(InitialEventKind::Patch,std::int64_t(half_line)*even_half_lines);
}
bool InitialBootEventOwner::Dispatch(const InitialBootEvent& event, std::int64_t late) {
    switch (event.kind) {
    case InitialEventKind::Dtk: {
        Require(event.userdata==DtkUserdata && !state_.ai.playing &&
                !state_.dvd.stream,"unproved DTK branch");
        // DTK reply transfer_size0 preserves DIMAR/DILENGTH, creates no DI IRQ.
        auto& dvd=state_.dvd;
        Record("dtk-transfer-zero","-", {dvd.dimar,dvd.dilength});
        ++dvd.streaming_push_calls;
        dvd.streaming_frames += dvd.pending_blocks*28u;
        Record("dtk-zero-sample-request","-", {dvd.pending_blocks,0u,dvd.pending_blocks*28u});
        // num_samples0 bypasses both outcomes of output-rate-valid without
        // PushSample/Enqueue; logging is a separate live read even at count0.
        if (!branches_.dtk_audio_logging) { stop_=InitialBootStop::DtkAudioLogRead; return false; }
        if (*branches_.dtk_audio_logging) { stop_=InitialBootStop::DtkAudioLogBranch; return false; }
        Record("dtk-no-wave-write","-",{0u});
        dvd.pending_blocks=6u;
        Record("dtk-pending-blocks","-",{dvd.pending_blocks});
        const auto period=std::int64_t(config_.cpu_hz)*dvd.pending_blocks*28u*
            state_.ai.ais_divisor/108000000u;
        Schedule(InitialEventKind::Dtk,period-late,DtkUserdata);
        return true;
    }
    case InitialEventKind::Gpu:
        Record("gpu-allow-sleep-request","-",{1u});
        stop_=InitialBootStop::GpuSleepEffect;
        if (!branches_.gpu_allow_sleep) return false;
        branches_.gpu_allow_sleep(); // throwing sink preserves the exact pending effect
        stop_=InitialBootStop::Advancing;
        state_.gpu_sleep_effect_delivered=true; ++state_.gpu_allow_sleep_calls;
        Record("gpu-allow-sleep-delivered","-",{1u});
        Schedule(InitialEventKind::Gpu,std::int64_t(config_.cpu_hz/1000u)-late);
        return true;
    case InitialEventKind::Dsp:
        Require(state_.dsp.hle_rom && !state_.dsp.dma_enabled,"unproved DSP branch");
        ++state_.dsp.update_calls;
        state_.dsp.last_update_cycles=static_cast<std::uint32_t>(config_.cpu_hz/1000u-late);
        Record("dsp-rom-update","-", {state_.dsp.last_update_cycles,state_.dsp.rom_mail});
        Schedule(InitialEventKind::Dsp,std::int64_t(config_.cpu_hz/1000u)-late);
        return true;
    case InitialEventKind::Vi: {
        auto& vi=state_.vi;
        const auto ticks=state_.global_timer-late;
        const auto odd_lines=3u*vi.equalization+vi.odd_prb+2u*vi.active_lines+vi.odd_psb;
        const auto even_lines=3u*vi.equalization+vi.even_prb+2u*vi.active_lines+vi.even_psb;
        Record("vt-update","-", {Raw(ticks),vi.half_line,vi.next_si_poll,odd_lines,even_lines});
        Require(vi.half_line==0u,"finite VI owner only supports first update");
        // VI.cpp957 calls FrameUpdate before rendering/NewField/polling.
        if (!state_.movie.frame) { stop_=InitialBootStop::MovieFrameCounterRead; return false; }
        ++*state_.movie.frame;
        if (!*state_.movie.polled) ++*state_.movie.lag;
        // FreshInactive is an explicit control binding, never a live default.
        state_.movie.polled=false;
        Record("movie-frame-update","-", {*state_.movie.frame,*state_.movie.lag,0u});
        Require(vi.half_line!=vi.odd_first && vi.half_line!=vi.even_first &&
                vi.half_line!=vi.odd_last && vi.half_line!=vi.even_last,
                "unproved BeginField/EndField branch");
        ++state_.new_field_calls;
        if (!branches_.frame_step) { stop_=InitialBootStop::NewFieldFrameStepRead; return false; }
        if (*branches_.frame_step) { stop_=InitialBootStop::NewFieldFrameStepBranch; return false; }
        Record("new-field-no-step","-", {0u});
        if (!branches_.achievement_client_present) {
            stop_=InitialBootStop::AchievementClientRead; return false;
        }
        if (*branches_.achievement_client_present) {
            stop_=InitialBootStop::AchievementClientBranch; return false;
        }
        if (!branches_.achievement_dll_found) { stop_=InitialBootStop::AchievementDllRead; return false; }
        if (*branches_.achievement_dll_found) { stop_=InitialBootStop::AchievementDllBranch; return false; }
        // IsGameLoaded(null)==false and dll_found=false short-circuit DoFrame
        // before CPU-thread checks, rc_client_do_frame or host steady_clock.
        ++state_.achievement_return_calls;
        Record("achievement-return","-", {0u,0u});
        Require(vi.half_line!=vi.next_si_poll,"unproved first-update SI poll");
        vi.next_si_poll=vi.half_line+15u;
        Record("vt-rebase","-", {Raw(ticks),vi.half_line,vi.next_si_poll,odd_lines,even_lines});
        ++vi.half_line;
        if (vi.half_line==odd_lines+even_lines) vi.half_line=0u;
        Record("vt-advanced","-", {Raw(ticks),vi.half_line,vi.next_si_poll,odd_lines,even_lines});
        if (!(vi.half_line&1u)) vi.last_line_start=static_cast<std::uint64_t>(ticks);
        // Fresh CoreTiming member m_throttle_disable_vi_int=false (h217).
        // No prior throttle/presentation ingress is admitted in this capsule.
        bool asserted=false;
        for (auto& interrupt:vi.interrupts) {
            const auto target=interrupt.hct>vi.hlw ? 1u : 0u;
            if (1u+vi.half_line/2u==interrupt.vct && (vi.half_line&1u)==target)
                interrupt.asserted=true;
            asserted |= interrupt.asserted && interrupt.mask;
        }
        if (asserted) state_.pi.cause |= ViCause; else state_.pi.cause &= ~ViCause;
        state_.pi.exceptions=ProjectInitialPiException(
            state_.pi.cause,state_.pi.mask,state_.pi.exceptions);
        Record("pi-vi-update","-", {state_.pi.cause,state_.pi.mask,state_.pi.exceptions});
        const auto period=(2u*config_.cpu_hz/27000000u)*vi.hlw;
        Schedule(InitialEventKind::Vi,std::int64_t(period)-late);
        return true;
    }
    case InitialEventKind::Audio:
    case InitialEventKind::Patch:
        throw std::invalid_argument("callback outside first-Advance capsule");
    }
    throw std::invalid_argument("unknown callback");
}
InitialBootStop InitialBootEventOwner::FirstAdvance(const InitialAdvanceAuthorization& authorization) {
    Require(!attempted_,"finite first Advance may only be attempted once");
    Require(authorization.configuration_unchanged==true,"unknown/changed configuration");
    Require(authorization.foreign_queue_empty==true,"unknown/foreign scheduler ingress");
    Require(authorization.no_device_or_guest_writes==true,"unknown/device/guest writer ingress");
    attempted_=true;
    stop_=InitialBootStop::Advancing;
    ClockRecord("advance-enter");
    const auto executed=state_.slice_length-state_.downcount;
    state_.global_timer+=executed;
    state_.slice_length=MaxSlice; state_.sane=true;
    ClockRecord("advance-clock",static_cast<std::uint64_t>(executed));
    while (!state_.queue.empty() && state_.queue.front().deadline<=state_.global_timer) {
        const auto event=state_.queue.front();
        state_.queue.erase(state_.queue.begin()); state_.active_callback=event;
        Record("dispatch",InitialEventName(event.kind),
               {Raw(event.deadline),event.fifo,event.userdata,Raw(state_.global_timer-event.deadline)});
        if (!Dispatch(event,state_.global_timer-event.deadline)) {
            Record("stop",InitialBootStopName(stop_));
            return stop_;
        }
        Record("callback-return",InitialEventName(event.kind),
               {Raw(event.deadline),event.fifo,event.userdata});
        state_.active_callback.reset();
    }
    state_.sane=false;
    if (!state_.queue.empty())
        state_.slice_length=static_cast<std::int32_t>(
            std::min<std::int64_t>(state_.queue.front().deadline-state_.global_timer,MaxSlice));
    state_.downcount=state_.slice_length;
    ClockRecord("advance-exit");
    // Fresh PI mask0 preserved zero exceptions; CheckExternalExceptions returns.
    Require(state_.pi.exceptions==0u,"unproved external exception path");
    Record("external-exception-return","-",{state_.pi.exceptions});
    stop_=InitialBootStop::FirstAdvanceComplete;
    return stop_;
}
} // namespace shadow::boot
