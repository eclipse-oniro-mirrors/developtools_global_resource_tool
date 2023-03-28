/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef OHOS_RESTOOL_RESOURCE_DATA_H
#define OHOS_RESTOOL_RESOURCE_DATA_H

#include<map>
#include<stdint.h>
#include<string>

namespace OHOS {
namespace Global {
namespace Restool {
const static std::string TOOL_NAME = "restool";
const static std::string RESOURCES_DIR = "resources";
const static std::string CONFIG_JSON = "config.json";
const static std::string MODULE_JSON = "module.json";
const static std::string RAW_FILE_DIR = "rawfile";
const static std::string ID_DEFINED_FILE = "id_defined.json";
const static std::string RESOURCE_INDEX_FILE = "resources.index";
const static std::string SEPARATOR = "/";
const static std::string WIN_SEPARATOR = "\\";
const static std::string NEW_LINE_PATH = "\r\nat ";
const static std::string LONG_PATH_HEAD = "\\\\?\\";
const static int32_t VERSION_MAX_LEN = 128;
const static int32_t INT_TO_BYTES = sizeof(uint32_t);
static const int8_t RESTOOL_VERSION[VERSION_MAX_LEN] = { "Restool 2.010" };
const static int32_t TAG_LEN = 4;

enum class KeyType {
    LANGUAGE = 0,
    REGION = 1,
    RESOLUTION = 2,
    ORIENTATION = 3,
    DEVICETYPE = 4,
    SCRIPT = 5,
    NIGHTMODE = 6,
    MCC = 7,
    MNC = 8,
    // RESERVER 9
    INPUTDEVICE = 10,
    KEY_TYPE_MAX,
};

enum class ResType {
    ELEMENT = 0,
    ANIMATION = 1,
    LAYOUT = 3,
    RAW = 6,
    INTEGER = 8,
    STRING = 9,
    STRARRAY = 10,
    INTARRAY = 11,
    BOOLEAN = 12,
    COLOR = 14,
    ID = 15,
    THEME = 16,
    PLURAL = 17,
    FLOAT = 18,
    MEDIA = 19,
    PROF = 20,
    GRAPHIC = 21,
    PATTERN = 22,
    INVALID_RES_TYPE = -1,
};

enum class OrientationType {
    VERTICAL = 0,
    HORIZONTAL = 1,
};

const std::map<std::string, OrientationType> g_orientaionMap = {
    { "vertical", OrientationType::VERTICAL },
    { "horizontal", OrientationType::HORIZONTAL },
};

enum class DeviceType {
    PHONE = 0,
    TABLET = 1,
    CAR = 2,
    // RESERVER 3
    TV = 4,
    WEARABLE = 6,
};

const std::map<std::string, DeviceType> g_deviceMap = {
    { "phone", DeviceType::PHONE },
    { "tablet", DeviceType::TABLET },
    { "car", DeviceType::CAR },
    { "tv", DeviceType::TV },
    { "wearable", DeviceType::WEARABLE },
};

enum class ResolutionType {
    SDPI = 120,
    MDPI = 160,
    LDPI = 240,
    XLDPI = 320,
    XXLDPI = 480,
    XXXLDPI = 640,
};

const std::map<std::string, ResolutionType> g_resolutionMap = {
    { "sdpi", ResolutionType::SDPI },
    { "mdpi",  ResolutionType::MDPI },
    { "ldpi",  ResolutionType::LDPI },
    { "xldpi", ResolutionType::XLDPI },
    { "xxldpi", ResolutionType::XXLDPI },
    { "xxxldpi", ResolutionType::XXXLDPI },
};

enum class NightMode {
    DARK = 0,
    LIGHT = 1,
};

const std::map<std::string, NightMode> g_nightModeMap = {
    { "dark", NightMode::DARK },
    { "light", NightMode::LIGHT },
};

enum class InputDevice {
    INPUTDEVICE_NOT_SET = -1,
    INPUTDEVICE_POINTINGDEVICE = 0,
};

const std::map<std::string, InputDevice> g_inputDeviceMap = {
    { "pointingdevice", InputDevice::INPUTDEVICE_POINTINGDEVICE },
};

struct KeyParam {
    KeyType keyType;
    uint32_t value;
};

struct IdData {
    uint32_t id;
    uint32_t dataOffset;
};

const std::map<std::string, ResType> g_fileClusterMap = {
    { "element", ResType::ELEMENT },
    { "media", ResType::MEDIA },
    { "profile", ResType::PROF },
};

const std::map<std::string, ResType> g_contentClusterMap = {
    { "id", ResType::ID },
    { "integer", ResType::INTEGER },
    { "string", ResType::STRING },
    { "strarray", ResType::STRARRAY },
    { "intarray", ResType::INTARRAY },
    { "color", ResType::COLOR },
    { "plural", ResType::PLURAL },
    { "boolean", ResType::BOOLEAN },
    { "pattern", ResType::PATTERN },
    { "theme", ResType::THEME },
    { "float", ResType::FLOAT }
};

const std::map<int32_t, ResType> g_resTypeMap = {
    { static_cast<int32_t>(ResType::ELEMENT), ResType::ELEMENT},
    { static_cast<int32_t>(ResType::ANIMATION), ResType::ANIMATION},
    { static_cast<int32_t>(ResType::LAYOUT), ResType::LAYOUT},
    { static_cast<int32_t>(ResType::RAW), ResType::RAW},
    { static_cast<int32_t>(ResType::INTEGER), ResType::INTEGER},
    { static_cast<int32_t>(ResType::STRING), ResType::STRING},
    { static_cast<int32_t>(ResType::STRARRAY), ResType::STRARRAY},
    { static_cast<int32_t>(ResType::INTARRAY), ResType::INTARRAY},
    { static_cast<int32_t>(ResType::BOOLEAN), ResType::BOOLEAN},
    { static_cast<int32_t>(ResType::COLOR), ResType::COLOR},
    { static_cast<int32_t>(ResType::ID), ResType::ID},
    { static_cast<int32_t>(ResType::THEME), ResType::THEME},
    { static_cast<int32_t>(ResType::PLURAL), ResType::PLURAL},
    { static_cast<int32_t>(ResType::FLOAT), ResType::FLOAT},
    { static_cast<int32_t>(ResType::MEDIA), ResType::MEDIA},
    { static_cast<int32_t>(ResType::PROF), ResType::PROF},
    { static_cast<int32_t>(ResType::GRAPHIC), ResType::GRAPHIC},
    { static_cast<int32_t>(ResType::PATTERN), ResType::PATTERN},
    { static_cast<int32_t>(ResType::INVALID_RES_TYPE), ResType::INVALID_RES_TYPE},
};

struct DirectoryInfo {
    std::string limitKey;
    std::string fileCluster;
    std::string dirPath;
    std::vector<KeyParam> keyParams;
    ResType dirType;
};

struct FileInfo : DirectoryInfo {
    std::string filePath;
    std::string filename;
    ResType fileType;
};
}
}
}
#endif
