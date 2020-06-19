/*
 * Copyright 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <android/hardware/tv/tuner/1.0/types.h>
#include <binder/MemoryDealer.h>
#include <hidl/HidlSupport.h>
#include <hidl/HidlTransportSupport.h>
#include <hidl/Status.h>
#include <hidlmemory/FrameworkUtils.h>

using android::hardware::tv::tuner::V1_0::DataFormat;
using android::hardware::tv::tuner::V1_0::DemuxFilterEvent;
using android::hardware::tv::tuner::V1_0::DemuxFilterMainType;
using android::hardware::tv::tuner::V1_0::DemuxFilterSettings;
using android::hardware::tv::tuner::V1_0::DemuxFilterType;
using android::hardware::tv::tuner::V1_0::DemuxRecordScIndexType;
using android::hardware::tv::tuner::V1_0::DemuxTpid;
using android::hardware::tv::tuner::V1_0::DemuxTsFilterType;
using android::hardware::tv::tuner::V1_0::DvrSettings;
using android::hardware::tv::tuner::V1_0::DvrType;
using android::hardware::tv::tuner::V1_0::FrontendDvbtBandwidth;
using android::hardware::tv::tuner::V1_0::FrontendDvbtCoderate;
using android::hardware::tv::tuner::V1_0::FrontendDvbtConstellation;
using android::hardware::tv::tuner::V1_0::FrontendDvbtGuardInterval;
using android::hardware::tv::tuner::V1_0::FrontendDvbtHierarchy;
using android::hardware::tv::tuner::V1_0::FrontendDvbtSettings;
using android::hardware::tv::tuner::V1_0::FrontendDvbtStandard;
using android::hardware::tv::tuner::V1_0::FrontendDvbtTransmissionMode;
using android::hardware::tv::tuner::V1_0::FrontendSettings;
using android::hardware::tv::tuner::V1_0::FrontendStatus;
using android::hardware::tv::tuner::V1_0::FrontendStatusType;
using android::hardware::tv::tuner::V1_0::FrontendType;
using android::hardware::tv::tuner::V1_0::PlaybackSettings;
using android::hardware::tv::tuner::V1_0::RecordSettings;

using namespace std;

const uint32_t FMQ_SIZE_1M = 0x100000;
const uint32_t FMQ_SIZE_4M = 0x400000;
const uint32_t FMQ_SIZE_16M = 0x1000000;

#define CLEAR_KEY_SYSTEM_ID 0xF6D8
#define PROVISION_STR                                      \
    "{                                                   " \
    "  \"id\": 21140844,                                 " \
    "  \"name\": \"Test Title\",                         " \
    "  \"lowercase_organization_name\": \"Android\",     " \
    "  \"asset_key\": {                                  " \
    "  \"encryption_key\": \"nezAr3CHFrmBR9R8Tedotw==\"  " \
    "  },                                                " \
    "  \"cas_type\": 1,                                  " \
    "  \"track_types\": [ ]                              " \
    "}                                                   "

typedef enum {
    TS_VIDEO0,
    TS_VIDEO1,
    TS_AUDIO0,
    TS_PES0,
    TS_PCR0,
    TS_SECTION0,
    TS_TS0,
    TS_RECORD0,
    FILTER_MAX,
} Filter;

typedef enum {
    DVBT,
    DVBS,
    FRONTEND_MAX,
} Frontend;

typedef enum {
    SCAN_DVBT,
    SCAN_MAX,
} FrontendScan;

typedef enum {
    DVR_RECORD0,
    DVR_PLAYBACK0,
    DVR_MAX,
} Dvr;

typedef enum {
    DESC_0,
    DESC_MAX,
} Descrambler;

struct FilterConfig {
    uint32_t bufferSize;
    DemuxFilterType type;
    DemuxFilterSettings settings;

    bool operator<(const FilterConfig& /*c*/) const { return false; }
};

struct FrontendConfig {
    FrontendType type;
    FrontendSettings settings;
    vector<FrontendStatusType> tuneStatusTypes;
    vector<FrontendStatus> expectTuneStatuses;
};

struct ChannelConfig {
    int32_t frontendId;
    int32_t channelId;
    std::string channelName;
    DemuxTpid videoPid;
    DemuxTpid audioPid;
};

struct DvrConfig {
    DvrType type;
    uint32_t bufferSize;
    DvrSettings settings;
    string playbackInputFile;
};

struct DescramblerConfig {
    uint32_t casSystemId;
    string provisionStr;
    vector<uint8_t> hidlPvtData;
};

static FrontendConfig frontendArray[FILTER_MAX];
static FrontendConfig frontendScanArray[SCAN_MAX];
static ChannelConfig channelArray[FRONTEND_MAX];
static FilterConfig filterArray[FILTER_MAX];
static DvrConfig dvrArray[DVR_MAX];
static DescramblerConfig descramblerArray[DESC_MAX];
static vector<string> goldenOutputFiles;

/** Configuration array for the frontend tune test */
inline void initFrontendConfig() {
    FrontendDvbtSettings dvbtSettings{
            .frequency = 578000,
            .transmissionMode = FrontendDvbtTransmissionMode::AUTO,
            .bandwidth = FrontendDvbtBandwidth::BANDWIDTH_8MHZ,
            .constellation = FrontendDvbtConstellation::AUTO,
            .hierarchy = FrontendDvbtHierarchy::AUTO,
            .hpCoderate = FrontendDvbtCoderate::AUTO,
            .lpCoderate = FrontendDvbtCoderate::AUTO,
            .guardInterval = FrontendDvbtGuardInterval::AUTO,
            .isHighPriority = true,
            .standard = FrontendDvbtStandard::T,
    };
    frontendArray[DVBT].type = FrontendType::DVBT, frontendArray[DVBT].settings.dvbt(dvbtSettings);
    vector<FrontendStatusType> types;
    types.push_back(FrontendStatusType::DEMOD_LOCK);
    FrontendStatus status;
    status.isDemodLocked(true);
    vector<FrontendStatus> statuses;
    statuses.push_back(status);
    frontendArray[DVBT].tuneStatusTypes = types;
    frontendArray[DVBT].expectTuneStatuses = statuses;
    frontendArray[DVBS].type = FrontendType::DVBS;
};

/** Configuration array for the frontend scan test */
inline void initFrontendScanConfig() {
    frontendScanArray[SCAN_DVBT].type = FrontendType::DVBT;
    frontendScanArray[SCAN_DVBT].settings.dvbt({
            .frequency = 578000,
            .transmissionMode = FrontendDvbtTransmissionMode::MODE_8K,
            .bandwidth = FrontendDvbtBandwidth::BANDWIDTH_8MHZ,
            .constellation = FrontendDvbtConstellation::AUTO,
            .hierarchy = FrontendDvbtHierarchy::AUTO,
            .hpCoderate = FrontendDvbtCoderate::AUTO,
            .lpCoderate = FrontendDvbtCoderate::AUTO,
            .guardInterval = FrontendDvbtGuardInterval::AUTO,
            .isHighPriority = true,
            .standard = FrontendDvbtStandard::T,
    });
};

/** Configuration array for the filter test */
inline void initFilterConfig() {
    // TS VIDEO filter setting for default implementation testing
    filterArray[TS_VIDEO0].type.mainType = DemuxFilterMainType::TS;
    filterArray[TS_VIDEO0].type.subType.tsFilterType(DemuxTsFilterType::VIDEO);
    filterArray[TS_VIDEO0].bufferSize = FMQ_SIZE_16M;
    filterArray[TS_VIDEO0].settings.ts().tpid = 256;
    filterArray[TS_VIDEO0].settings.ts().filterSettings.av({.isPassthrough = false});
    filterArray[TS_VIDEO1].type.mainType = DemuxFilterMainType::TS;
    filterArray[TS_VIDEO1].type.subType.tsFilterType(DemuxTsFilterType::VIDEO);
    filterArray[TS_VIDEO1].bufferSize = FMQ_SIZE_16M;
    filterArray[TS_VIDEO1].settings.ts().tpid = 256;
    filterArray[TS_VIDEO1].settings.ts().filterSettings.av({.isPassthrough = false});
    // TS AUDIO filter setting
    filterArray[TS_AUDIO0].type.mainType = DemuxFilterMainType::TS;
    filterArray[TS_AUDIO0].type.subType.tsFilterType(DemuxTsFilterType::AUDIO);
    filterArray[TS_AUDIO0].bufferSize = FMQ_SIZE_16M;
    filterArray[TS_AUDIO0].settings.ts().tpid = 256;
    filterArray[TS_AUDIO0].settings.ts().filterSettings.av({.isPassthrough = false});
    // TS PES filter setting
    filterArray[TS_PES0].type.mainType = DemuxFilterMainType::TS;
    filterArray[TS_PES0].type.subType.tsFilterType(DemuxTsFilterType::PES);
    filterArray[TS_PES0].bufferSize = FMQ_SIZE_16M;
    filterArray[TS_PES0].settings.ts().tpid = 256;
    filterArray[TS_PES0].settings.ts().filterSettings.pesData({
            .isRaw = false,
            .streamId = 0xbd,
    });
    // TS PCR filter setting
    filterArray[TS_PCR0].type.mainType = DemuxFilterMainType::TS;
    filterArray[TS_PCR0].type.subType.tsFilterType(DemuxTsFilterType::PCR);
    filterArray[TS_PCR0].bufferSize = FMQ_SIZE_16M;
    filterArray[TS_PCR0].settings.ts().tpid = 256;
    filterArray[TS_PCR0].settings.ts().filterSettings.noinit();
    // TS filter setting
    filterArray[TS_TS0].type.mainType = DemuxFilterMainType::TS;
    filterArray[TS_TS0].type.subType.tsFilterType(DemuxTsFilterType::TS);
    filterArray[TS_TS0].bufferSize = FMQ_SIZE_16M;
    filterArray[TS_TS0].settings.ts().tpid = 256;
    filterArray[TS_TS0].settings.ts().filterSettings.noinit();
    // TS SECTION filter setting
    filterArray[TS_SECTION0].type.mainType = DemuxFilterMainType::TS;
    filterArray[TS_SECTION0].type.subType.tsFilterType(DemuxTsFilterType::SECTION);
    filterArray[TS_SECTION0].bufferSize = FMQ_SIZE_16M;
    filterArray[TS_SECTION0].settings.ts().tpid = 256;
    filterArray[TS_SECTION0].settings.ts().filterSettings.section({
            .isRaw = false,
    });
    // TS RECORD filter setting
    filterArray[TS_RECORD0].type.mainType = DemuxFilterMainType::TS;
    filterArray[TS_RECORD0].type.subType.tsFilterType(DemuxTsFilterType::RECORD);
    filterArray[TS_RECORD0].settings.ts().tpid = 81;
    filterArray[TS_RECORD0].settings.ts().filterSettings.record({
            .scIndexType = DemuxRecordScIndexType::NONE,
    });
};

/** Configuration array for the dvr test */
inline void initDvrConfig() {
    RecordSettings recordSettings{
            .statusMask = 0xf,
            .lowThreshold = 0x1000,
            .highThreshold = 0x07fff,
            .dataFormat = DataFormat::TS,
            .packetSize = 188,
    };
    dvrArray[DVR_RECORD0].type = DvrType::RECORD;
    dvrArray[DVR_RECORD0].bufferSize = FMQ_SIZE_4M;
    dvrArray[DVR_RECORD0].settings.record(recordSettings);
    PlaybackSettings playbackSettings{
            .statusMask = 0xf,
            .lowThreshold = 0x1000,
            .highThreshold = 0x07fff,
            .dataFormat = DataFormat::TS,
            .packetSize = 188,
    };
    dvrArray[DVR_PLAYBACK0].type = DvrType::PLAYBACK;
    dvrArray[DVR_PLAYBACK0].playbackInputFile = "/vendor/etc/segment000000.ts";
    dvrArray[DVR_PLAYBACK0].bufferSize = FMQ_SIZE_4M;
    dvrArray[DVR_PLAYBACK0].settings.playback(playbackSettings);
};

/** Configuration array for the descrambler test */
inline void initDescramblerConfig() {
    descramblerArray[DESC_0].casSystemId = CLEAR_KEY_SYSTEM_ID;
    descramblerArray[DESC_0].provisionStr = PROVISION_STR;
    descramblerArray[DESC_0].hidlPvtData.resize(256);
};
