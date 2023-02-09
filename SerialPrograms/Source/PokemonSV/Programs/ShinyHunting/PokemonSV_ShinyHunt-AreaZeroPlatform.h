/*  Shiny Hunt - Area Zero Platform
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#ifndef PokemonAutomation_PokemonSV_ShinyHuntAreaZeroPlatform_H
#define PokemonAutomation_PokemonSV_ShinyHuntAreaZeroPlatform_H

#include <functional>
#include "Common/Cpp/Options/BooleanCheckBoxOption.h"
#include "Common/Cpp/Options/SimpleIntegerOption.h"
#include "Common/Cpp/Options/EnumDropdownOption.h"
#include "CommonFramework/Notifications/EventNotificationsTable.h"
#include "NintendoSwitch/NintendoSwitch_SingleSwitchProgram.h"
#include "NintendoSwitch/Options/NintendoSwitch_GoHomeWhenDoneOption.h"
#include "PokemonSV_AreaZeroPlatform.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonSV{

class EncounterWatcher;



class ShinyHuntAreaZeroPlatform_Descriptor : public SingleSwitchProgramDescriptor{
public:
    ShinyHuntAreaZeroPlatform_Descriptor();

    struct Stats;
    virtual std::unique_ptr<StatsTracker> make_stats() const override;
};


class ShinyHuntAreaZeroPlatform : public SingleSwitchProgramInstance{
public:
    ShinyHuntAreaZeroPlatform();

    virtual void program(SingleSwitchProgramEnvironment& env, BotBaseContext& context) override;

private:
    void on_shiny_encounter(
        ProgramEnvironment& env, BotBaseContext& context,
        const EncounterWatcher& encounter_watcher
    );

    void run_iteration(ProgramEnvironment& env, ConsoleHandle& console, BotBaseContext& context);
    bool clear_in_front(
        ProgramEnvironment& env, ConsoleHandle& console, BotBaseContext& context,
        std::function<void(BotBaseContext& context)>&& command
    );

    void run_path0(ProgramEnvironment& env, ConsoleHandle& console, BotBaseContext& context);
    void run_path1(ProgramEnvironment& env, ConsoleHandle& console, BotBaseContext& context);
    void run_path2(ProgramEnvironment& env, ConsoleHandle& console, BotBaseContext& context);



private:
    enum class Mode{
        START_ON_PLATFORM_NO_RESET,
        START_IN_ZERO_GATE_NO_RESET,
        START_IN_ZERO_GATE_PERIODIC_RESET,
    };
    EnumDropdownOption<Mode> MODE;

    SimpleIntegerOption<uint16_t> RESET_DURATION_MINUTES;

    enum class Path{
        PATH0,
        PATH1,
        PATH2,
    };
    EnumDropdownOption<Path> PATH;

    BooleanCheckBoxOption VIDEO_ON_SHINY;

    NavigatePlatformSettings NAVIGATE_TO_PLATFORM;

    GoHomeWhenDoneOption GO_HOME_WHEN_DONE;

    EventNotificationOption NOTIFICATION_STATUS_UPDATE;
    EventNotificationOption NOTIFICATION_SHINY;
    EventNotificationsOption NOTIFICATIONS;

    uint64_t m_iterations = 0;
};





}
}
}
#endif
