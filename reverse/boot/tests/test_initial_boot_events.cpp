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
void Dump(const InitialBootEventOwner& owner, bool bound, const char* control=nullptr, bool owned=false) {
    const auto& s=owner.State();
    std::cout << "{\"schema\":\"initial-boot-events-42-v1\",\"scope\":\"research-only\","
              << "\"control\":\"" << (control ? control : (bound ? "declared-bound-control" : "unbound-reference"))
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
              << "}";
    if (owned) {
        std::cout << ",\"dtk_logging_owner\":{\"constructor_owned\":" << s.dtk_logging.constructor_owned
                  << ",\"configuration_owned\":" << s.dtk_logging.configuration_owned
                  << ",\"flag_read\":" << s.dtk_logging.flag_read
                  << ",\"enabled\":" << s.dtk_logging.enabled
                  << ",\"backend_sample_rate\":" << s.dtk_logging.backend_sample_rate << "}";
    }
    std::cout << ",\"journal\":[";
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
unsigned OwnedDtkTests() {
    using Inputs=InitialDtkLoggingSourceInputs;
    const auto config=FreshInitialBootSourceConfig42();
    const auto inputs=FreshInitialDtkLoggingSource42();
    auto owned=InitialBootEventOwner::WithOwnedDtkLogging(config,inputs);
    Require(owned.State().dtk_logging.constructor_owned && !owned.State().dtk_logging.enabled &&
            owned.State().dtk_logging.backend_sample_rate==48000u,"owned source constructor missing");
    Require(owned.FirstAdvance(Authorized())==InitialBootStop::GpuSleepEffect,"owned DTK did not reach GPU boundary");
    const auto& s=owned.State();
    Require(s.dtk_logging.configuration_owned && s.dtk_logging.flag_read && !s.dtk_logging.enabled &&
            s.global_timer==20000 && s.slice_length==20000 && s.downcount==0 && s.sane &&
            s.next_fifo==7u && s.queue.size()==5u && s.active_callback->kind==InitialEventKind::Gpu &&
            s.dvd.pending_blocks==6u && s.dvd.streaming_push_calls==1u && s.dvd.streaming_frames==0u &&
            s.pi.cause==0x10100u && s.pi.mask==0u && s.pi.exceptions==0u &&
            !s.gpu_sleep_effect_delivered && s.gpu_allow_sleep_calls==0u && s.dsp.update_calls==0u &&
            s.vi.half_line==0u && !s.movie.frame && s.guest_ram_write_bytes==0u,
            "owned DTK partial state fabricated later effects");
    const auto dtk=std::find_if(s.queue.begin(),s.queue.end(),[](const auto& e){return e.kind==InitialEventKind::Dtk;});
    Require(dtk!=s.queue.end() && dtk->deadline==1699488 && dtk->fifo==6u &&
            dtk->userdata==0x300000001ull,"owned source successor wrong");
    // Old declared false control has the same semantic prefix. Owned rows
    // expose new provenance; all former records retain their exact bytes.
    InitialBootBranchBindings control;control.dtk_audio_logging=false;
    InitialBootEventOwner declared(config,control);declared.FirstAdvance(Authorized());
    std::vector<InitialEventRecord> semantic;
    for (const auto& r:owned.Journal()) if (r.kind.rfind("dtk-owned-",0)!=0) semantic.push_back(r);
    Require(semantic.size()==declared.Journal().size(),"owned logging changed semantic journal size");
    for (std::size_t n=0;n<semantic.size();++n)
        Require(semantic[n].kind==declared.Journal()[n].kind && semantic[n].name==declared.Journal()[n].name &&
                semantic[n].values==declared.Journal()[n].values,"owned logging changed existing semantic effect order");
    Decline([&]{owned.FirstAdvance(Authorized());});
    unsigned declines=1;
    for (unsigned field=0;field<4;++field) for (auto ingress:{Inputs::Ingress::Unknown,Inputs::Ingress::Present}) {
        auto input=inputs;
        auto* target=field==0 ? &input.changed_configuration : field==1 ? &input.logging_requests :
                     field==2 ? &input.mutable_config_aliases : &input.lifetime_changes;
        *target=ingress;
        auto candidate=InitialBootEventOwner::WithOwnedDtkLogging(config,input);
        const auto expected=field==0 ? InitialBootStop::DtkConfigIngressRead :
            field==1 ? InitialBootStop::DtkLogRequestIngressRead :
            field==2 ? InitialBootStop::DtkMutableConfigAliasRead : InitialBootStop::DtkMixerLifetimeIngressRead;
        Require(candidate.FirstAdvance(Authorized())==expected,"unknown/present logging ingress consumed");
        const auto& p=candidate.State();
        if (field==1) {
            Require(p.global_timer==20000 && p.active_callback->kind==InitialEventKind::Dtk &&
                    p.next_fifo==6u && p.dvd.pending_blocks==0u && p.dvd.streaming_push_calls==1u &&
                    !p.dtk_logging.flag_read,"unknown Start/Stop ingress advanced past live read");
        } else {
            Require(p.global_timer==0 && !p.active_callback && p.next_fifo==6u &&
                    !p.dtk_logging.configuration_owned && !p.dtk_logging.flag_read,
                    "unknown config/alias/lifetime mutated elapsed time");
        }
        ++declines;
    }
    auto unknown_lifecycle=inputs;unknown_lifecycle.mixer_lifecycle=Inputs::MixerLifecycle::Unknown;
    auto lifecycle=InitialBootEventOwner::WithOwnedDtkLogging(config,unknown_lifecycle);
    Require(lifecycle.FirstAdvance(Authorized())==InitialBootStop::DtkMixerLifecycleRead &&
            !lifecycle.State().dtk_logging.constructor_owned && lifecycle.State().global_timer==0,
            "unknown constructor/lifetime fabricated initial member");++declines;
    auto unknown_config=inputs;unknown_config.configuration=Inputs::Configuration::Unknown;
    auto configuration=InitialBootEventOwner::WithOwnedDtkLogging(config,unknown_config);
    Require(configuration.FirstAdvance(Authorized())==InitialBootStop::DtkConfigRead &&
            !configuration.State().dtk_logging.configuration_owned && configuration.State().global_timer==0,
            "unknown resolver result treated as default false");++declines;
    auto enabled_input=inputs;enabled_input.configuration=Inputs::Configuration::InitialDumpAudioEnabled;
    auto enabled=InitialBootEventOwner::WithOwnedDtkLogging(config,enabled_input);
    Require(enabled.FirstAdvance(Authorized())==InitialBootStop::DtkAudioDumpStartedRead &&
            enabled.State().dtk_logging.constructor_owned && !enabled.State().dtk_logging.enabled &&
            !enabled.State().dtk_logging.flag_read && enabled.State().global_timer==0 &&
            enabled.State().next_fifo==6u && enabled.State().dvd.streaming_push_calls==0u,
            "enabled logging invented WAV Start success");++declines;
    Decline([&]{InitialBootEventOwner::WithOwnedDtkLogging(config,inputs,control);});++declines;
    for (unsigned field=0;field<6;++field) {
        auto invalid=inputs;
        if (field==0) invalid.mixer_lifecycle=static_cast<Inputs::MixerLifecycle>(99);
        else if (field==1) invalid.configuration=static_cast<Inputs::Configuration>(99);
        else {
            auto* target=field==2 ? &invalid.changed_configuration : field==3 ? &invalid.logging_requests :
                         field==4 ? &invalid.mutable_config_aliases : &invalid.lifetime_changes;
            *target=static_cast<Inputs::Ingress>(99);
        }
        Decline([&]{InitialBootEventOwner::WithOwnedDtkLogging(config,invalid);});++declines;
    }
    return declines;
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
    negatives+=OwnedDtkTests();
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
        if (argc==2 && (std::string(argv[1])=="--dump-owned-dtk" ||
                        std::string(argv[1])=="--dump-owned-dtk-enabled")) {
            auto inputs=FreshInitialDtkLoggingSource42();
            const bool enabled=std::string(argv[1])=="--dump-owned-dtk-enabled";
            if (enabled) inputs.configuration=InitialDtkLoggingSourceInputs::Configuration::InitialDumpAudioEnabled;
            auto owner=InitialBootEventOwner::WithOwnedDtkLogging(FreshInitialBootSourceConfig42(),inputs);
            owner.FirstAdvance(Authorized());
            Dump(owner,false,enabled ? "owned-initial-dump-enabled" : "owned-fresh-dtk-source",true);
            return 0;
        }
        Require(argc==1,"unknown first-Advance test option");
        Tests(); return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
