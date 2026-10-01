#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace shadow::boot {

// Finite source-specific RESEARCH owner. No guest execution or host time.
enum class InitialEventKind { Dtk, Gpu, Vi, Dsp, Audio, Patch };
struct InitialBootEvent {
    std::int64_t deadline{};
    std::uint64_t fifo{};
    InitialEventKind kind{};
    std::uint64_t userdata{};
};
bool InitialEventBefore(const InitialBootEvent&, const InitialBootEvent&);
const char* InitialEventName(InitialEventKind);
// ProcessorInterface.cpp181..187; preserves every unrelated pending flag.
std::uint32_t ProjectInitialPiException(std::uint32_t cause, std::uint32_t mask,
                                        std::uint32_t exceptions);

struct InitialBootSourceConfig {
    enum class Profile { Unknown, FreshGcPalHle42 };
    Profile profile = Profile::Unknown;
    std::uint32_t cpu_hz{};
    std::uint32_t scheduler_factor_bits{};
    std::uint32_t vi_factor_bits{};
    bool restored_state{};
    bool extra_initial_events{};
};
// Config values are audited source inputs, not observer queue outputs.
InitialBootSourceConfig FreshInitialBootSourceConfig42();

struct InitialBootBranchBindings {
    enum class MovieLifecycle { Unknown, FreshInactive };
    MovieLifecycle movie_lifecycle = MovieLifecycle::Unknown;
    // A known false is a declared control until independently source-bound.
    std::optional<bool> dtk_audio_logging;
    // Typed external effect sink: caller owns the BlockingLoop/worker lifecycle.
    // Delivery does not assert the worker's later atomic flag value.
    std::function<void()> gpu_allow_sleep;
    std::optional<bool> frame_step;
    std::optional<bool> achievement_client_present;
    std::optional<bool> achievement_dll_found;
};
struct InitialAdvanceAuthorization {
    std::optional<bool> configuration_unchanged;
    std::optional<bool> foreign_queue_empty;
    std::optional<bool> no_device_or_guest_writes;
};

enum class InitialBootStop {
    NotAdvanced, Advancing, DtkAudioLogRead, DtkAudioLogBranch, GpuSleepEffect,
    MovieFrameCounterRead, NewFieldFrameStepRead,
    NewFieldFrameStepBranch, AchievementClientRead, AchievementClientBranch,
    AchievementDllRead, AchievementDllBranch, FirstAdvanceComplete
};
const char* InitialBootStopName(InitialBootStop);
struct InitialEventRecord {
    std::string kind;
    std::string name;
    std::array<std::uint64_t, 8> values{};
};
struct InitialBootSnapshot {
    std::int64_t global_timer{};
    std::int32_t slice_length{}, downcount{};
    bool sane{};
    std::uint64_t next_fifo{};
    std::uint32_t inverse_factor_bits{};
    std::vector<InitialBootEvent> queue;
    std::optional<InitialBootEvent> active_callback;

    struct Pi { std::uint32_t cause{}, mask{}, exceptions{}; } pi;
    struct Dvd {
        bool stream{}, stop_at_track_end{};
        std::uint32_t dimar{}, dilength{}, pending_blocks{};
        std::uint32_t audio_position{}, next_start{}, next_length{}, current_start{}, current_length{};
        std::uint32_t decoded_blocks{}, streaming_push_calls{}, streaming_frames{};
    } dvd;
    struct Ai { bool playing{}; std::uint32_t ais_divisor{}, aid_divisor{}; } ai;
    struct Dsp {
        bool hle_rom{}, dma_enabled{}, mail_halted{};
        std::uint32_t rom_mail{}, dma_source{}, dma_blocks{}, dsp_slice{};
        std::uint32_t update_calls{}, last_update_cycles{};
    } dsp;
    struct ViInterrupt { std::uint32_t hct{}, vct{}; bool mask{}, asserted{}; };
    struct Vi {
        std::uint32_t half_line{}, next_si_poll{}, hlw{}, clock{};
        std::uint32_t equalization{}, active_lines{}, odd_prb{}, odd_psb{}, even_prb{}, even_psb{};
        std::uint32_t odd_first{}, odd_last{}, even_first{}, even_last{};
        std::uint64_t last_line_start{};
        std::array<ViInterrupt,4> interrupts{};
    } vi;
    struct Movie {
        std::optional<std::uint64_t> frame, lag, total_frames, total_lag;
        std::optional<bool> polled;
    } movie;
    bool gpu_sleep_effect_delivered{};
    std::uint32_t gpu_allow_sleep_calls{}, new_field_calls{}, achievement_return_calls{};
    std::uint32_t guest_ram_write_bytes{};
};

// Unknown live controls stop at their first actual read. Partial callback
// mutations remain owned; a stopped capsule cannot be resumed by inserting
// observed fields. Construct a new independent control to test another path.
class InitialBootEventOwner {
public:
    explicit InitialBootEventOwner(const InitialBootSourceConfig&,
                                   const InitialBootBranchBindings& = {});
    InitialBootStop FirstAdvance(const InitialAdvanceAuthorization&);
    const InitialBootSnapshot& State() const { return state_; }
    const std::vector<InitialEventRecord>& Journal() const { return journal_; }
    InitialBootStop Stop() const { return stop_; }
private:
    void Record(const char*, const char*, std::array<std::uint64_t,8> = {});
    void ClockRecord(const char*, std::uint64_t = 0);
    void Schedule(InitialEventKind, std::int64_t, std::uint64_t = 0);
    bool Dispatch(const InitialBootEvent&, std::int64_t);
    InitialBootSourceConfig config_;
    InitialBootBranchBindings branches_;
    InitialBootSnapshot state_;
    std::vector<InitialEventRecord> journal_;
    InitialBootStop stop_ = InitialBootStop::NotAdvanced;
    bool attempted_{};
};

} // namespace shadow::boot
