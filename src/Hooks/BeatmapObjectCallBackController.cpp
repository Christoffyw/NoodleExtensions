#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"

#include "GlobalNamespace/BeatmapObjectCallbackController.hpp"
#include "GlobalNamespace/BeatmapObjectCallbackData.hpp"
#include "GlobalNamespace/BeatmapEventCallbackData.hpp"
#include "GlobalNamespace/BeatmapEventCallback.hpp"
#include "GlobalNamespace/BeatmapObjectCallback.hpp"
#include "GlobalNamespace/BeatmapLineData.hpp"
#include "GlobalNamespace/BeatmapObjectData.hpp"
#include "System/Collections/Generic/HashSet_1.hpp"
#include "System/Action.hpp"

#include "custom-json-data/shared/CustomBeatmapData.h"
#include "Animation/Events.h"
#include "AssociatedData.h"
#include "tracks/shared/TimeSourceHelper.h"
#include "NEHooks.h"
#include "NELogger.h"
#include "SharedUpdate.h"

using namespace GlobalNamespace;

MAKE_HOOK_MATCH(BeatmapObjectCallbackController_LateUpdate, &BeatmapObjectCallbackController::LateUpdate, void, BeatmapObjectCallbackController *self) {
    if (!Hooks::isNoodleHookEnabled())
        return BeatmapObjectCallbackController_LateUpdate(self);

    if (!self->beatmapData) {
        return;
    }

    NESharedUpdate::TriggerUpdate();

    static auto *customObstacleDataClass = classof(CustomJSONData::CustomObstacleData *);
    static auto *customNoteDataClass = classof(CustomJSONData::CustomNoteData *);
    static auto callback_mptr = il2cpp_utils::FindMethodUnsafe("", "BeatmapObjectSpawnController", "HandleBeatmapObjectCallback", 1)->methodPointer;

    float songTime = TimeSourceHelper::getSongTime(self->audioTimeSource);
    auto *beatmapLinesData = reinterpret_cast<Array<BeatmapLineData *> *>(self->beatmapData->get_beatmapLinesData());

    for (int i = 0; i < self->beatmapObjectCallbackData->size; i++) {
        self->beatmapObjectDataCallbackCacheList->Clear();
        BeatmapObjectCallbackData *callbackData = self->beatmapObjectCallbackData->items.get(i);
        for (int j = 0; j < beatmapLinesData->Length(); j++) {
            while (callbackData->nextObjectIndexInLine.get(j) < ((List<GlobalNamespace::BeatmapObjectData*>*) beatmapLinesData->values[j]->get_beatmapObjectsData())->get_Count()) {
                BeatmapObjectData *beatmapObjectData = beatmapLinesData->values[j]->get_beatmapObjectsData()->get_Item(callbackData->nextObjectIndexInLine.get(j));

                float aheadTime = callbackData->aheadTime;
                if (callbackData->callback->method_ptr.m_value == callback_mptr) {
                    if (beatmapObjectData->klass == customObstacleDataClass) {
                        auto obstacleData = (CustomJSONData::CustomObstacleData *) beatmapObjectData;
                        aheadTime = getAD(obstacleData->customData).aheadTime;
                    } else if (beatmapObjectData->klass == customNoteDataClass) {
                        auto noteData = (CustomJSONData::CustomNoteData *) beatmapObjectData;
                        aheadTime = getAD(noteData->customData).aheadTime;
                    }
                }
                // NELogger::GetLogger().info("Method name: %s", callbackData->callback->method_info->get_Name());

                if (beatmapObjectData->time - aheadTime >= songTime) {
                    break;
                }
                if (beatmapObjectData->time >= self->spawningStartTime) {
                    for (int k = self->beatmapObjectDataCallbackCacheList->get_Count(); k >= 0; k--) {
                        if (k == 0 || self->beatmapObjectDataCallbackCacheList->items.get(k - 1)->time <= beatmapObjectData->time) {
                            self->beatmapObjectDataCallbackCacheList->Insert(k, beatmapObjectData);
                            break;
                        }
                    }
                }
                callbackData->nextObjectIndexInLine.get(j)++;
            }
        }
        for (int j = 0; j < self->beatmapObjectDataCallbackCacheList->size; j++) {
            callbackData->callback->Invoke(self->beatmapObjectDataCallbackCacheList->items.get(j));
        }
    }

    for (int l = 0; l < self->beatmapEventCallbackData->get_Count(); l++) {
        BeatmapEventCallbackData *callbackData = self->beatmapEventCallbackData->items.get(l);
        while (callbackData->nextEventIndex < ((List<GlobalNamespace::BeatmapEventData*>*) self->beatmapData->get_beatmapEventsData())->get_Count()) {
            BeatmapEventData *beatmapEventData = self->beatmapData->get_beatmapEventsData()->get_Item(callbackData->nextEventIndex);
            if (beatmapEventData->time - callbackData->aheadTime >= songTime) {
                break;
            }
            if (self->validEvents->Contains(beatmapEventData->type)) {
                callbackData->callback->Invoke(beatmapEventData);
            }
            callbackData->nextEventIndex++;
        }
    }
    while (self->nextEventIndex < ((List<GlobalNamespace::BeatmapEventData*>*) self->beatmapData->get_beatmapEventsData())->get_Count()) {
        BeatmapEventData *beatmapEventData = self->beatmapData->get_beatmapEventsData()->get_Item(self->nextEventIndex);
        if (beatmapEventData->time >= songTime) {
            break;
        }
        self->SendBeatmapEventDidTriggerEvent(beatmapEventData);
        self->nextEventIndex++;
    }
    if (self->callbacksForThisFrameWereProcessedEvent) {
        self->callbacksForThisFrameWereProcessedEvent->Invoke();
    }

}


void InstallBeatmapObjectCallbackControllerHooks(Logger& logger) {
    INSTALL_HOOK_ORIG(logger, BeatmapObjectCallbackController_LateUpdate);
}

NEInstallHooks(InstallBeatmapObjectCallbackControllerHooks);