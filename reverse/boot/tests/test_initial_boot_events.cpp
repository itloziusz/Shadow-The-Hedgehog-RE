#include "shadow/boot/InitialBootEvents.hpp"

#include <algorithm>
#include <atomic>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {
using namespace shadow::boot;
void Require(bool valid, const char* message) { if (!valid) throw std::runtime_error(message); }
template<class F> void Decline(F f) {
    try { f(); } catch (const std::invalid_argument&) { return; }
    throw std::runtime_error("unsupported input accepted");
}
InitialAdvanceAuthorization Authorized() { return {true,true,true}; }
InitialBootBranchBindings Bound(std::atomic<bool>& gpu) {
    InitialBootBranchBindings b;
    b.dtk_audio_logging=false;
    b.gpu_allow_sleep=[&gpu] { gpu.store(true); };
    b.movie_lifecycle=InitialBootBranchBindings::MovieLifecycle::FreshInactive;
    b.frame_step=false; b.achievement_client_present=false; b.achievement_dll_found=false;
    return b;
}
void EventJson(const InitialBootEvent& event) {
    std::cout << "{\"name\":\"" << InitialEventName(event.kind) << "\",\"deadline\":"
              << event.deadline << ",\"fifo\":" << event.fifo
              << ",\"userdata\":\"" << std::hex << std::setw(16) << std::setfill('0')
              << event.userdata << std::dec << "\"}";
}
template<class T> void OptionalJson(const std::optional<T>& value) {
    if (value) std::cout << +*value; else std::cout << "null";
}
void Dump(const InitialBootEventOwner& owner, bool bound) {
    const auto& s=owner.State();
    std::cout << "{\"schema\":\"initial-boot-events-42-v1\",\"scope\":\"research-only\","
              << "\"control\":\"" << (bound ? "declared-bound-control" : "unbound-reference")
              << "\",\"stop\":\"" << InitialBootStopName(owner.Stop())
              << "\",\"scheduler\":{\"global\":" << s.global_timer << ",\"slice\":" << s.slice_length
              << ",\"downcount\":" << s.downcount << ",\"sane\":" << s.sane
              << ",\"next_fifo\":" << s.next_fifo << ",\"inverse_bits\":"
              << s.inverse_factor_bits << "},\"queue\":[";
    bool first=true;
    for (const auto& event:s.queue) { if (!first) std::cout << ','; first=false; EventJson(event); }
    std::cout << "],\"active_callback\":";
    if (s.active_callback) EventJson(*s.active_callback); else std::cout << "null";
    std::cout << ",\"pi\":{\"cause\":" << s.pi.cause << ",\"mask\":" << s.pi.mask
              << ",\"exceptions\":" << s.pi.exceptions << "},\"dvd\":{\"dimar\":" << s.dvd.dimar
              << ",\"dilength\":" << s.dvd.dilength << ",\"stream\":" << s.dvd.stream
              << ",\"pending_blocks\":" << s.dvd.pending_blocks << ",\"decoded_blocks\":" << s.dvd.decoded_blocks
              << ",\"push_calls\":" << s.dvd.streaming_push_calls
              << ",\"streaming_frames\":" << s.dvd.streaming_frames << "},\"ai\":{\"playing\":"
              << s.ai.playing << ",\"ais_divisor\":" << s.ai.ais_divisor
              << ",\"aid_divisor\":" << s.ai.aid_divisor << "},\"dsp\":{\"hle_rom\":"
              << s.dsp.hle_rom << ",\"dma_enabled\":" << s.dsp.dma_enabled
              << ",\"mail_halted\":" << s.dsp.mail_halted << ",\"rom_mail\":" << s.dsp.rom_mail
              << ",\"slice\":" << s.dsp.dsp_slice << ",\"update_calls\":" << s.dsp.update_calls
              << ",\"last_update_cycles\":" << s.dsp.last_update_cycles << "},\"vi\":{\"half_line\":"
              << s.vi.half_line << ",\"next_si_poll\":" << s.vi.next_si_poll
              << ",\"last_line_start\":" << s.vi.last_line_start << ",\"odd_first\":" << s.vi.odd_first
              << ",\"odd_last\":" << s.vi.odd_last << ",\"even_first\":" << s.vi.even_first
              << ",\"even_last\":" << s.vi.even_last << "},\"movie\":{\"frame\":";
    OptionalJson(s.movie.frame); std::cout << ",\"lag\":"; OptionalJson(s.movie.lag);
    std::cout << ",\"polled\":"; OptionalJson(s.movie.polled);
    std::cout << ",\"total_frames\":"; OptionalJson(s.movie.total_frames);
    std::cout << ",\"total_lag\":"; OptionalJson(s.movie.total_lag);
    std::cout << "},\"effects\":{\"gpu_sleep_delivered\":" << s.gpu_sleep_effect_delivered
              << ",\"gpu_allow_sleep_calls\":" << s.gpu_allow_sleep_calls
              << ",\"new_field_calls\":" << s.new_field_calls << ",\"achievement_return_calls\":"
              << s.achievement_return_calls << ",\"guest_ram_write_bytes\":" << s.guest_ram_write_bytes
              << "},\"journal\":[";
    first=true;
    for (const auto& record:owner.Journal()) {
        if (!first) std::cout << ','; first=false;
        std::cout << "{\"kind\":\"" << record.kind << "\",\"name\":\"" << record.name << "\",\"v\":[";
        for (unsigned n=0;n<8;++n) {
            if (n) std::cout << ',';
            std::cout << '"' << std::hex << std::setw(16) << std::setfill('0') << record.values[n] << std::dec << '"';
        }
        std::cout << "]}";
    }
    std::cout << "]}\n";
}
void Tests() {
    Require(ProjectInitialPiException(0x100u,0x100u,0x10u)==0x14u,
            "PI must set exception bit4 and preserve bit10");
    Require(ProjectInitialPiException(0x100u,0u,0x14u)==0x10u &&
            ProjectInitialPiException(0u,0x100u,0x14u)==0x10u,
            "PI must clear only bit4 under either no-cause/no-mask path");
    Require(ProjectInitialPiException(1u,1u,0x12345678u)==(0x12345678u|4u) &&
            ProjectInitialPiException(1u,0u,0x1234567cu)==(0x1234567cu&~4u),
            "PI projection lost unrelated pending flags");
    const auto config=FreshInitialBootSourceConfig42();
    InitialBootEventOwner unknown(config);
    Require(unknown.State().queue.size()==6u && unknown.State().next_fifo==6u,"fresh queue missing");
    Require(unknown.State().downcount==0 && unknown.State().slice_length==20000 &&
            unknown.State().sane && unknown.State().global_timer==0,"fresh scheduler wrong");
    Require(unknown.FirstAdvance(Authorized())==InitialBootStop::DtkAudioLogRead,"unbound log read ignored");
    Require(unknown.State().active_callback->kind==InitialEventKind::Dtk &&
            unknown.State().dvd.pending_blocks==0u && unknown.State().queue.size()==5u &&
            unknown.State().next_fifo==6u,"unbound DTK mutated successors");
    Decline([&] { unknown.FirstAdvance(Authorized()); });

    std::atomic<bool> gpu{false};
    auto branches=Bound(gpu);
    InitialBootEventOwner bound(config,branches);
    Require(bound.FirstAdvance(Authorized())==InitialBootStop::FirstAdvanceComplete,"bound control did not finish");
    const auto& s=bound.State();
    Require(s.global_timer==20000 && s.slice_length==10888 && s.downcount==10888 &&
            !s.sane && s.next_fifo==10u && !s.active_callback,"first Advance phase wrong");
    Require(s.pi.cause==0x10000u && s.pi.mask==0u && s.pi.exceptions==0u,"first VI PI effect missing");
    Require(s.dvd.pending_blocks==6u && s.dvd.dimar==0u && s.dvd.dilength==0u &&
            s.dvd.streaming_frames==0u && s.dvd.decoded_blocks==0u,"DTK zero transfer wrong");
    Require(gpu.load() && s.gpu_sleep_effect_delivered && s.gpu_allow_sleep_calls==1u,"GPU effect not delivered");
    Require(s.dsp.rom_mail==0x8071feedu && s.dsp.mail_halted && !s.dsp.dma_enabled &&
            s.dsp.last_update_cycles==466000u && s.dsp.dsp_slice==0u,"ROM update changed state");
    Require(s.vi.half_line==1u && s.vi.next_si_poll==15u && s.vi.last_line_start==0u &&
            s.movie.frame==1u && s.movie.lag==1u && s.movie.polled==false &&
            s.movie.total_frames==0u && s.movie.total_lag==0u,"first field effects missing");
    Require(s.vi.odd_first==520u && s.vi.odd_last==519u &&
            s.vi.even_first==1045u && s.vi.even_last==1044u,"VI PSB correction lost");
    const std::array<std::int64_t,6> times{30888,121392,486000,486000,1699488,8108100};
    const std::array<std::uint64_t,6> fifos{9,4,7,8,6,5};
    for (unsigned n=0;n<6;++n)
        Require(s.queue[n].deadline==times[n] && s.queue[n].fifo==fifos[n],"signed queue/FIFO wrong");
    std::vector<InitialEventKind> dispatch;
    for (const auto& record:bound.Journal()) if (record.kind=="dispatch")
        dispatch.push_back(record.name=="FinishExecutingCommand" ? InitialEventKind::Dtk :
                           record.name=="GPUSleeper" ? InitialEventKind::Gpu :
                           record.name=="DSPCallback" ? InitialEventKind::Dsp : InitialEventKind::Vi);
    Require(dispatch==std::vector<InitialEventKind>({InitialEventKind::Dtk,InitialEventKind::Gpu,
                                                 InitialEventKind::Dsp,InitialEventKind::Vi}),
            "deadline/FIFO dispatch order wrong");
    std::vector<InitialBootEvent> signed_items{{0,0,InitialEventKind::Gpu,0},
        {-1,99,InitialEventKind::Dtk,0},{0,1,InitialEventKind::Dsp,0}};
    std::sort(signed_items.begin(),signed_items.end(),InitialEventBefore);
    Require(signed_items[0].deadline==-1 && signed_items[1].fifo==0 &&
            signed_items[2].fifo==1,"negative deadline or equal-time FIFO lost");

    unsigned negatives=0;
    auto reject_config=[&](auto alter) { auto c=config; alter(c); Decline([&] {InitialBootEventOwner owner(c);}); ++negatives; };
    reject_config([](auto& c) {c.profile=InitialBootSourceConfig::Profile::Unknown;});
    reject_config([](auto& c) {++c.cpu_hz;});
    reject_config([](auto& c) {c.scheduler_factor_bits=0u;});
    reject_config([](auto& c) {c.vi_factor_bits=0x40000000u;});
    reject_config([](auto& c) {c.restored_state=true;});
    reject_config([](auto& c) {c.extra_initial_events=true;});
    for (unsigned field=0;field<3;++field) for (bool known_false:{false,true}) {
        auto a=Authorized(); auto* value=field==0 ? &a.configuration_unchanged :
            field==1 ? &a.foreign_queue_empty : &a.no_device_or_guest_writes;
        if (known_false) *value=false; else value->reset();
        InitialBootEventOwner owner(config,branches);
        Decline([&] {owner.FirstAdvance(a);}); ++negatives;
        Require(owner.State().global_timer==0 && owner.State().next_fifo==6u,"declined ingress mutated owner");
    }
    for (unsigned stage=0;stage<8;++stage) {
        auto b=Bound(gpu);
        const std::array<InitialBootStop,8> stops{InitialBootStop::DtkAudioLogBranch,
            InitialBootStop::GpuSleepEffect,InitialBootStop::MovieFrameCounterRead,
            InitialBootStop::NewFieldFrameStepRead,InitialBootStop::NewFieldFrameStepBranch,
            InitialBootStop::AchievementClientRead,InitialBootStop::AchievementClientBranch,
            InitialBootStop::AchievementDllRead};
        if (stage==0) b.dtk_audio_logging=true;
        if (stage==1) b.gpu_allow_sleep={};
        if (stage==2) b.movie_lifecycle=InitialBootBranchBindings::MovieLifecycle::Unknown;
        if (stage==3) b.frame_step.reset();
        if (stage==4) b.frame_step=true;
        if (stage==5) b.achievement_client_present.reset();
        if (stage==6) b.achievement_client_present=true;
        if (stage==7) b.achievement_dll_found.reset();
        InitialBootEventOwner owner(config,b);
        Require(owner.FirstAdvance(Authorized())==stops[stage],"unknown/unsupported branch was consumed");
        Require(owner.State().pi.cause==0x10100u,"partial VI applied later PI effect");
        ++negatives;
    }
    branches.achievement_dll_found=true;
    InitialBootEventOwner dll(config,branches);
    Require(dll.FirstAdvance(Authorized())==InitialBootStop::AchievementDllBranch,"active DLL accepted");
    ++negatives;
    auto throwing=Bound(gpu);
    throwing.gpu_allow_sleep=[] { throw std::runtime_error("controlled sink failure"); };
    InitialBootEventOwner partial(config,throwing);
    bool propagated=false;
    try { partial.FirstAdvance(Authorized()); }
    catch (const std::runtime_error&) { propagated=true; }
    Require(propagated && partial.Stop()==InitialBootStop::GpuSleepEffect &&
            partial.State().active_callback->kind==InitialEventKind::Gpu &&
            partial.State().dvd.pending_blocks==6u && partial.State().next_fifo==7u &&
            !partial.State().gpu_sleep_effect_delivered && partial.State().gpu_allow_sleep_calls==0u,
            "throwing GPU effect fabricated completion");
    Decline([&] {partial.FirstAdvance(Authorized());}); ++negatives;
    std::cout << "PASS first-Advance research owner; " << negatives
              << " input/branch declines; signed/FIFO and ordered effects; unbound stops at Mixer.cpp253\n";
}
}
int main(int argc,char** argv) {
    try {
        if (argc==2 && (std::string(argv[1])=="--dump" || std::string(argv[1])=="--dump-bound-control")) {
            const bool bound=std::string(argv[1])=="--dump-bound-control";
            std::atomic<bool> gpu{false};
            InitialBootEventOwner owner(FreshInitialBootSourceConfig42(),
                bound ? Bound(gpu) : InitialBootBranchBindings{});
            owner.FirstAdvance(Authorized()); Dump(owner,bound); return 0;
        }
        Require(argc==1,"unknown first-Advance test option");
        Tests(); return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
