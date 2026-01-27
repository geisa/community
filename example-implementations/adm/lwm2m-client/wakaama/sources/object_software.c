/*******************************************************************************
 *
 * Copyright (c) 2026 Southern California Edison.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Distribution License v1.0
 * and Apache License 2.0 which accompany this distribution.
 *
 * The Eclipse Distribution License is available at
 *    http://www.eclipse.org/org/documents/edl-v10.php.
 * The Apache License is available at
 *    http://www.apache.org/licenses/LICENSE-2.0.
 *
 * Contributors:
 *    Nghia Dam - initial implementation
 *    Kevin L'Hopital - initial implementation
 *
 *******************************************************************************/

/*
 * This object provides installation and activation of software functionality.
 * Object ID is 9.
 */

/*
 * resources:
 * 0 PkgName                            read
 * 1 PkgVersion                         read
 * 2 package                            write
 * 3 package uri                        write
 * 4 Install                            exec
 * 5 Checkpoint                         read
 * 6 Uninstall                          exec
 * 7 Update state                       read
 * 8 Update Supported Objects           read/write
 * 9 Update Result                      read
 * 10 Activate                          exec
 * 11 Deactivate                        exec
 * 12 Activation State                  read
 * 13 Package Settings                  read/write
 * 14 User Name                         write
 * 15 Password                          write
 * 16 Status Reason                     read
 * 17 Software Component Link           read
 * 18 Software Component tree length    read
 */

#include <liblwm2m.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "object_common.h"

// ---- private object "Software" specific defines ----
// Resource Id's:
#define RES_O_PKG_NAME                  0
#define RES_O_PKG_VERSION               1
#define RES_M_PACKAGE                   2
#define RES_M_PACKAGE_URI               3
#define RES_M_INSTALL                   4
#define RES_O_CHECKPOINT                5
#define RES_O_UNINSTALL                 6
#define RES_M_UPDATE_STATE              7
#define RES_M_UPDATE_SUPPORTED_OBJECTS  8
#define RES_M_UPDATE_RESULT             9
#define RES_O_ACTIVATE                 10
#define RES_O_DEACTIVATE               11
#define RES_M_ACTIVATION_STATE         12
#define RES_O_PACKAGE_SETTINGS         13
#define RES_O_USER_NAME                14
#define RES_O_PASSWORD                 15
#define RES_O_STATUS_REASON            16
#define RES_O_SW_COMPONENT_LINK        17
#define RES_O_SW_COMPONENT_TREE_LEN    18

#define LWM2M_SOFTWARE_UPDATE_OBJECT_ID 9

#ifndef GEISA_PACKAGE_SCRIPT_PATH
#define GEISA_PACKAGE_SCRIPT_PATH "/usr/sbin/manage_package.sh"
#endif

typedef enum {
    UPDATE_STATE_INITIAL = 0,
    UPDATE_STATE_DOWNLOAD_STARTED = 1,
    UPDATE_STATE_DOWNLOADED = 2,
    UPDATE_STATE_DELIVERED = 3,
    UPDATE_STATE_INSTALLED = 4
} software_update_state_t;

typedef enum {
    ACTIVATION_STATE_INACTIVE = 0,
    ACTIVATION_STATE_ACTIVE = 1,
} software_activation_state_t;

typedef enum {
    UPDATE_RESULT_INITIAL = 0,
    UPDATE_RESULT_DOWNLOADING = 1,
    UPDATE_RESULT_DOWNLOAD_ERROR = 2,
    UPDATE_RESULT_INTEGRITY_FAILURE = 3,
    UPDATE_RESULT_INSTALLATION_FAILURE = 4,
    UPDATE_RESULT_INSTALLATION_SUCCESS = 5
} software_update_result_t;

typedef struct {
    software_update_state_t update_state;
    software_update_result_t update_result;
    software_activation_state_t activation_state;
    char pkg_name[256];
    char pkg_version[256];
    char pkg_id[512];
    uint32_t persistent_storage_kibs;
    uint32_t non_persistent_storage_kibs;
} software_data_t;


static bool verify_package_integrity(const char *filepath) {
    return true;
}

static void prv_process_package(software_data_t *data, const uint8_t *buffer, size_t length) {
    const char *app_dir = "/platform/apps/geisa-app-1";
    const char *package_path = "/platform/apps/geisa-app-1/geisa-app-1-1.0.1.squashfs";

    if (access(app_dir, F_OK) != 0) {
        if (mkdir(app_dir, 0755) != 0) {
            data->update_state = UPDATE_STATE_INITIAL;
            data->update_result = UPDATE_RESULT_DOWNLOAD_ERROR;
            return;
        }
    }

    FILE *f = fopen(package_path, "wb");
    if (f) {
        fwrite(buffer, 1, length, f);
        fclose(f);

        strncpy(data->pkg_name, "geisa-app-1", sizeof(data->pkg_name) - 1);
        data->pkg_name[sizeof(data->pkg_name) - 1] = '\0';
        strncpy(data->pkg_version, "1.0.1", sizeof(data->pkg_version) - 1);
        data->pkg_version[sizeof(data->pkg_version) - 1] = '\0';
        strncpy(data->pkg_id, "org.lfenergy.geisa.geisa-app-1", sizeof(data->pkg_id) - 1);
        data->pkg_id[sizeof(data->pkg_id) - 1] = '\0';
        data->persistent_storage_kibs = 10240;      // 10 MB
        data->non_persistent_storage_kibs = 2048;   // 2 MB

        if (data->update_state == UPDATE_STATE_INITIAL) {
            data->update_state = UPDATE_STATE_DOWNLOAD_STARTED;
            data->update_result = UPDATE_RESULT_DOWNLOADING;
        }
        if (data->update_state == UPDATE_STATE_DOWNLOAD_STARTED) {
            data->update_state = UPDATE_STATE_DOWNLOADED;
            data->update_result = UPDATE_RESULT_INITIAL;

            if (verify_package_integrity(package_path) == true) {
                data->update_state = UPDATE_STATE_DELIVERED;
                data->update_result = UPDATE_RESULT_INITIAL;
            } else {
                data->update_state = UPDATE_STATE_INITIAL;
                data->update_result = UPDATE_RESULT_INTEGRITY_FAILURE;
            }
        }
    } else {
        data->update_state = UPDATE_STATE_INITIAL;
        data->update_result = UPDATE_RESULT_DOWNLOAD_ERROR;
    }
}

static uint8_t prv_software_read(lwm2m_context_t *contextP, uint16_t instanceId, int *numDataP,
                                 lwm2m_data_t **dataArrayP, lwm2m_object_t *objectP) {
    int i;
    uint8_t result;
    software_data_t *data = (software_data_t *)(objectP->userData);

    /* unused parameter */
    (void)contextP;

    // is the server asking for the full object ?
    if (*numDataP == 0) {
        *dataArrayP = lwm2m_data_new(4);
        if (*dataArrayP == NULL)
            return COAP_500_INTERNAL_SERVER_ERROR;
        *numDataP = 4;
        (*dataArrayP)[0].id = RES_M_UPDATE_STATE;
        (*dataArrayP)[1].id = RES_M_UPDATE_SUPPORTED_OBJECTS;
        (*dataArrayP)[2].id = RES_M_UPDATE_RESULT;
        (*dataArrayP)[3].id = RES_M_ACTIVATION_STATE;
    }

    i = 0;
    do {
        switch ((*dataArrayP)[i].id) {
        // Optional resources
        case RES_O_PKG_NAME:
        case RES_O_PKG_VERSION:
        case RES_O_CHECKPOINT:
        case RES_O_UNINSTALL:
        case RES_O_ACTIVATE:
        case RES_O_DEACTIVATE:
        case RES_O_PACKAGE_SETTINGS:
        case RES_O_USER_NAME:
        case RES_O_PASSWORD:
        case RES_O_STATUS_REASON:
        case RES_O_SW_COMPONENT_LINK:
        case RES_O_SW_COMPONENT_TREE_LEN:
        // Mandatory resources that do not support read
        case RES_M_INSTALL:
        case RES_M_PACKAGE:
        case RES_M_PACKAGE_URI:
            result = COAP_405_METHOD_NOT_ALLOWED;
            break;

        case RES_M_UPDATE_STATE:
            if ((*dataArrayP)[i].type == LWM2M_TYPE_MULTIPLE_RESOURCE)
                return COAP_404_NOT_FOUND;
            lwm2m_data_encode_int(data->update_state, *dataArrayP + i);
            result = COAP_205_CONTENT;
            break;

        case RES_M_UPDATE_RESULT:
            if ((*dataArrayP)[i].type == LWM2M_TYPE_MULTIPLE_RESOURCE)
                return COAP_404_NOT_FOUND;
            lwm2m_data_encode_int(data->update_result, *dataArrayP + i);
            result = COAP_205_CONTENT;
            break;

        case RES_M_ACTIVATION_STATE:
            if ((*dataArrayP)[i].type == LWM2M_TYPE_MULTIPLE_RESOURCE)
                return COAP_404_NOT_FOUND;
            lwm2m_data_encode_int(data->activation_state, *dataArrayP + i);
            result = COAP_205_CONTENT;
            break;
        default:
            result = COAP_404_NOT_FOUND;
        }

        i++;
    } while (i < *numDataP && result == COAP_205_CONTENT);
    return result;
}

static uint8_t prv_software_write(lwm2m_context_t *contextP, uint16_t instanceId, int numData, lwm2m_data_t *dataArray,
                                  lwm2m_object_t *objectP, lwm2m_write_type_t writeType) {
    int i;
    uint8_t result;
    software_data_t *data = (software_data_t *)(objectP->userData);

    /* unused parameters */
    (void)contextP;
    (void)writeType;

    i = 0;
    do {
        if (dataArray[i].type == LWM2M_TYPE_MULTIPLE_RESOURCE) {
            result = COAP_404_NOT_FOUND;
            i++;
            continue;
        }

        switch (dataArray[i].id) {
        case RES_M_PACKAGE:
            if (dataArray[i].type == LWM2M_TYPE_OPAQUE && dataArray[i].value.asBuffer.buffer != NULL) {
                prv_process_package(data, dataArray[i].value.asBuffer.buffer, dataArray[i].value.asBuffer.length);
                if (data->update_result == UPDATE_RESULT_DOWNLOADING || data->update_result == UPDATE_RESULT_INITIAL)
                    result = COAP_204_CHANGED;
                else if (data->update_result == UPDATE_RESULT_INTEGRITY_FAILURE || data->update_result == UPDATE_RESULT_DOWNLOAD_ERROR)
                    result = COAP_500_INTERNAL_SERVER_ERROR;
                else
                    result = COAP_204_CHANGED;
            } else {
                result = COAP_400_BAD_REQUEST;
            }
            break;

        case RES_M_PACKAGE_URI:
            fprintf(stdout, "\n\t PACKAGE URI (PULL) not implemented yet\r\n\n");
            result = COAP_204_CHANGED;
            break;
        default:
            result = COAP_405_METHOD_NOT_ALLOWED;
        }

        i++;
    } while (i < numData && result == COAP_204_CHANGED);

    return result;
}

static uint8_t prv_software_execute(lwm2m_context_t *contextP, uint16_t instanceId, uint16_t resourceId,
                                    uint8_t *buffer, int length, lwm2m_object_t *objectP) {
    software_data_t *data = (software_data_t *)(objectP->userData);
    uint8_t result;

    /* unused parameter */
    (void)contextP;

    if (length != 0)
        return COAP_400_BAD_REQUEST;

    switch (resourceId) {
    case RES_M_INSTALL:
        if (data->update_state == UPDATE_STATE_DELIVERED) {
            fprintf(stdout, "\n\t SOFTWARE INSTALLATION\r\n\n");

            char install_cmd[2048];
            snprintf(install_cmd, sizeof(install_cmd), "%s %s %s %s %s %u %u",
                    GEISA_PACKAGE_SCRIPT_PATH,
                    "install",
                    data->pkg_name,
                    data->pkg_version,
                    data->pkg_id,
                    data->persistent_storage_kibs,
                    data->non_persistent_storage_kibs);
            int ret = system(install_cmd);
            if (ret == 0) {
                data->update_state = UPDATE_STATE_INSTALLED;
                data->update_result = UPDATE_RESULT_INSTALLATION_SUCCESS;
                result = COAP_204_CHANGED;
            } else {
                data->update_result = UPDATE_RESULT_INSTALLATION_FAILURE;
                result = COAP_500_INTERNAL_SERVER_ERROR;
            }
        } else {
            // software installation already running
            result = COAP_400_BAD_REQUEST;
        }
        break;
    case RES_O_ACTIVATE:
        if (data->update_state == UPDATE_STATE_INSTALLED) {
            fprintf(stdout, "\n\t SOFTWARE ACTIVATION\r\n\n");

            char activate_cmd[1024];
            snprintf(activate_cmd, sizeof(activate_cmd), "%s %s %s",
                    GEISA_PACKAGE_SCRIPT_PATH,
                    "activate",
                    data->pkg_name);
            int ret = system(activate_cmd);
            if (ret == 0) {
                data->activation_state = ACTIVATION_STATE_ACTIVE;
                result = COAP_204_CHANGED;
            } else {
                result = COAP_500_INTERNAL_SERVER_ERROR;
            }
        } else {
            result = COAP_400_BAD_REQUEST;
        }
        break;
    case RES_O_DEACTIVATE:
        if (data->activation_state == ACTIVATION_STATE_ACTIVE) {
            fprintf(stdout, "\n\t SOFTWARE DEACTIVATION\r\n\n");
            char deactivate_cmd[1024];
            snprintf(deactivate_cmd, sizeof(deactivate_cmd), "%s %s %s",
                    GEISA_PACKAGE_SCRIPT_PATH,
                    "deactivate",
                    data->pkg_name);
            int ret = system(deactivate_cmd);
            if (ret == 0) {
                data->activation_state = ACTIVATION_STATE_INACTIVE;
                result = COAP_204_CHANGED;
            } else {
                result = COAP_500_INTERNAL_SERVER_ERROR;
            }
        } else {
            result = COAP_400_BAD_REQUEST;
        }
        break;
    case RES_O_UNINSTALL:
        fprintf(stdout, "\n\t SOFTWARE UNINSTALLATION\r\n\n");
        if (data->update_state != UPDATE_STATE_INSTALLED && data->update_state != UPDATE_STATE_DELIVERED) {
            result = COAP_400_BAD_REQUEST;
            break;
        }

        char uninstall_cmd[1024];
        snprintf(uninstall_cmd, sizeof(uninstall_cmd), "%s %s %s %s",
                GEISA_PACKAGE_SCRIPT_PATH,
                "uninstall",
                data->pkg_name,
                data->pkg_version);
        int ret = system(uninstall_cmd);
        if (ret == 0) {
            data->activation_state = ACTIVATION_STATE_INACTIVE;
            data->update_state = UPDATE_STATE_INITIAL;
            data->update_result = UPDATE_RESULT_INITIAL;
            result = COAP_204_CHANGED;
        } else {
            result = COAP_500_INTERNAL_SERVER_ERROR;
        }
        break;
    default:
        result = COAP_405_METHOD_NOT_ALLOWED;
        break;
    }

    return result;
}


void display_software_object(lwm2m_object_t *object) {
    software_data_t *data = (software_data_t *)object->userData;
    fprintf(stdout, "  /%u: Software Management object:\r\n", object->objID);
    if (NULL != data) {
        fprintf(stdout, "    state: %u, result: %u\r\n", data->update_state, data->update_result);
    }
}

lwm2m_object_t *get_object_software(void) {
    /*
     * The get_object_software function create the object itself and return a pointer to the structure that represent
     * it.
     */
    lwm2m_object_t *softwareObj;

    softwareObj = (lwm2m_object_t *)lwm2m_malloc(sizeof(lwm2m_object_t));

    if (NULL != softwareObj) {
        memset(softwareObj, 0, sizeof(lwm2m_object_t));

        softwareObj->objID = LWM2M_SOFTWARE_UPDATE_OBJECT_ID;

        softwareObj->instanceList = (lwm2m_list_t *)lwm2m_malloc(sizeof(lwm2m_list_t));
        if (NULL != softwareObj->instanceList) {
            memset(softwareObj->instanceList, 0, sizeof(lwm2m_list_t));
        } else {
            lwm2m_free(softwareObj);
            return NULL;
        }

        /*
         * And the private function that will access the object.
         * Those function will be called when a read/write/execute query is made by the server. In fact the library
         * don't need to know the resources of the object, only the server does.
         */
        softwareObj->readFunc = prv_software_read;
        softwareObj->writeFunc = prv_software_write;
        softwareObj->executeFunc = prv_software_execute;
        softwareObj->userData = lwm2m_malloc(sizeof(software_data_t));

        /*
         * Also some user data can be stored in the object with a private structure containing the needed variables
         */
        if (NULL != softwareObj->userData) {
            software_data_t *data = (software_data_t *)(softwareObj->userData);
            data->update_state = UPDATE_STATE_INITIAL;
            data->update_result = UPDATE_RESULT_INITIAL;
            data->activation_state = ACTIVATION_STATE_INACTIVE;
        } else {
            lwm2m_free(softwareObj);
            softwareObj = NULL;
        }
    }

    return softwareObj;
}

void free_object_software(lwm2m_object_t *objectP) {
    if(NULL == objectP) return;
    if (NULL != objectP->userData) {
        lwm2m_free(objectP->userData);
        objectP->userData = NULL;
    }
    if (NULL != objectP->instanceList) {
        lwm2m_free(objectP->instanceList);
        objectP->instanceList = NULL;
    }
    lwm2m_free(objectP);
}
