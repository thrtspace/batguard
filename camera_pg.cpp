
#include <SpinnakerC.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#if defined(__linux__)
#include <unistd.h>
#include <pthread.h>
#endif

// Compiler warning C4996 suppressed due to deprecated strcpy() and sprintf()
// functions on Windows platform.
#if defined WIN32 || defined _WIN32 || defined WIN64 || defined _WIN64
#pragma warning(disable : 4996)
#endif

typedef struct _acq_image {
    size_t width;
    size_t height;
    int type;
    int ready;
    size_t  size;
    char data[16*1024*1024];
} acq_image;

acq_image* g_images = NULL;
int g_images_count = 2;
int g_quit_flag = 0;
int g_camera_work=0;


// This macro helps with C-strings.
#define MAX_BUFF_LEN 256

// This function helps to check if a node is available and readable
bool8_t IsAvailableAndReadable(spinNodeHandle hNode, const char nodeName[])
{
    bool8_t pbAvailable = False;
    spinError err = SPINNAKER_ERR_SUCCESS;
    err = spinNodeIsAvailable(hNode, &pbAvailable);
    if (err != SPINNAKER_ERR_SUCCESS)
    {
        printf("Unable to retrieve node availability (%s node), with error %d...\n\n", nodeName, err);
    }

    bool8_t pbReadable = False;
    err = spinNodeIsReadable(hNode, &pbReadable);
    if (err != SPINNAKER_ERR_SUCCESS)
    {
        printf("Unable to retrieve node readability (%s node), with error %d...\n\n", nodeName, err);
    }
    return pbReadable && pbAvailable;
}


// This function helps to check if a node is available and writable
bool8_t IsAvailableAndWritable(spinNodeHandle hNode, const char nodeName[])
{
    bool8_t pbAvailable = False;
    spinError err = SPINNAKER_ERR_SUCCESS;
    err = spinNodeIsAvailable(hNode, &pbAvailable);
    if (err != SPINNAKER_ERR_SUCCESS)
    {
        printf("Unable to retrieve node availability (%s node), with error %d...\n\n", nodeName, err);
    }

    bool8_t pbWritable = False;
    err = spinNodeIsWritable(hNode, &pbWritable);
    if (err != SPINNAKER_ERR_SUCCESS)
    {
        printf("Unable to retrieve node writability (%s node), with error %d...\n\n", nodeName, err);
    }
    return pbWritable && pbAvailable;
}

// This function handles the error prints when a node or entry is unavailable or
// not readable/writable on the connected camera
void PrintRetrieveNodeFailure(const char node[], const char name[])
{
    printf("Unable to get %s (%s %s retrieval failed).\n\n", node, name, node);
}

// acq image thread
void* camera_image_thread(void* pdata)
{
    (void)pdata;

    spinSystem hSystem = NULL;
    spinCameraList hCameraList = NULL;
    spinCamera hCam = NULL;

    spinError err;

    spinNodeHandle hAcquisitionMode = NULL;
    spinNodeHandle hAcquisitionModeContinuous = NULL;
    int64_t acquisitionModeContinuous = 0;

    size_t numCameras = 0;
    spinNodeHandle handle;
    int64_t value;
    spinNodeMapHandle hNodeMapTLDevice = NULL;
    spinNodeMapHandle hNodeMap = NULL;

    g_camera_work = 0;

    // Retrieve singleton reference to system object

    err = spinSystemGetInstance(&hSystem);
    if (err != SPINNAKER_ERR_SUCCESS)
    {
        printf("Unable to retrieve system instance. Aborting with error %d...\n\n", err);
        goto cameraThreadClean;
    }

    // Retrieve list of cameras from the system


    err = spinCameraListCreateEmpty(&hCameraList);
    if (err != SPINNAKER_ERR_SUCCESS)
    {
        printf("Unable to create camera list. Aborting with error %d...\n\n", err);
        goto cameraThreadClean;
    }

    err = spinSystemGetCameras(hSystem, hCameraList);
    if (err != SPINNAKER_ERR_SUCCESS)
    {
        printf("Unable to retrieve camera list. Aborting with error %d...\n\n", err);
        goto cameraThreadClean;
    }

    // Retrieve number of cameras

    err = spinCameraListGetSize(hCameraList, &numCameras);
    if (err != SPINNAKER_ERR_SUCCESS)
    {
        printf("Unable to retrieve number of cameras. Aborting with error %d...\n\n", err);
        goto cameraThreadClean;
    }

    if(numCameras == 0)
    {
        printf("No camera detected\n");
        goto cameraThreadClean;
    }

    err = spinCameraListGet(hCameraList, 0, &hCam);
    if (err != SPINNAKER_ERR_SUCCESS)
    {
        printf("Unable to retrieve camera from list. Aborting with error %d...\n\n", err);
        goto cameraThreadClean;
    }


    // Retrieve TL device nodemap
    err = spinCameraGetTLDeviceNodeMap(hCam, &hNodeMapTLDevice);
    if (err != SPINNAKER_ERR_SUCCESS)
    {
        printf("Unable to retrieve TL device nodemap. Non-fatal error %d...\n\n", err);
        goto cameraThreadClean;
    }

    // Initialize camera
    err = spinCameraInit(hCam);
    if (err != SPINNAKER_ERR_SUCCESS)
    {
        printf("Unable to initialize camera. Aborting with error %d...\n\n", err);
        goto cameraThreadClean;
    }


    // Retrieve GenICam nodemap
    err = spinCameraGetNodeMap(hCam, &hNodeMap);
    if (err != SPINNAKER_ERR_SUCCESS)
    {
        printf("Unable to retrieve GenICam nodemap. Aborting with error %d...\n\n", err);
        goto cameraThreadClean;
    }


    // Retrieve enumeration node from nodemap
    err = spinNodeMapGetNode(hNodeMap, "AcquisitionMode", &hAcquisitionMode);
    if (err != SPINNAKER_ERR_SUCCESS)
    {
        printf("Unable to set acquisition mode to continuous (node retrieval). Aborting with error %d...\n\n", err);
        goto cameraThreadClean;
    }

    //set width/height

    err =spinNodeMapGetNode(hNodeMap, "GainAutoBalance", &handle);
    spinEnumerationSetEnumValue(handle, GainAutoBalance_Continuous);

    err = spinNodeMapGetNode(hNodeMap, "WidthMax", &handle);
    err = spinIntegerGetValue(handle, &value);
    printf("WidthMax = %ld\n", value);
    err = spinNodeMapGetNode(hNodeMap, "OffsetX", &handle);
    //err = spinIntegerSetValue(handle, value/2);
    printf("OffsetX = %ld\n", value);

    err = spinNodeMapGetNode(hNodeMap, "HeightMax", &handle);
    err = spinIntegerGetValue(handle, &value);
    printf("HeightMax = %ld\n", value);
    err = spinNodeMapGetNode(hNodeMap, "OffsetY", &handle);
    //err = spinIntegerSetValue(handle, value/2);
    printf("OffsetY = %ld\n", value);

    err = spinNodeMapGetNode(hNodeMap, "Width", &handle);
    err = spinIntegerSetValue(handle, 2592);

    err = spinNodeMapGetNode(hNodeMap, "Height", &handle);
    err = spinIntegerSetValue(handle, 1944);

    err = spinNodeMapGetNode(hNodeMap, "PixelFormat", &handle);

    // Retrieve entry node from enumeration node
    if (IsAvailableAndReadable(hAcquisitionMode, "AcquisitionMode"))
    {
        err = spinEnumerationGetEntryByName(hAcquisitionMode, "Continuous", &hAcquisitionModeContinuous);
        if (err != SPINNAKER_ERR_SUCCESS)
        {
            printf("Unable to set acquisition mode to continuous (entry 'continuous' retrieval). Aborting with error %d...\n\n", err);
            goto cameraThreadClean;
        }
    }
    else
    {
        PrintRetrieveNodeFailure("entry", "AcquisitionMode");
        goto cameraThreadClean;
    }

    // Retrieve integer from entry node

    if (IsAvailableAndReadable(hAcquisitionModeContinuous, "AcquisitionModeContinuous"))
    {
        err = spinEnumerationEntryGetIntValue(hAcquisitionModeContinuous, &acquisitionModeContinuous);
        if (err != SPINNAKER_ERR_SUCCESS)
        {
            printf("Unable to set acquisition mode to continuous (entry int value retrieval). Aborting with error %d...\n\n", err);
            goto cameraThreadClean;
        }
    }
    else
    {
        PrintRetrieveNodeFailure("entry", "AcquisitionMode 'Continuous'");
        goto cameraThreadClean;
    }

    // Set integer as new value of enumeration node
    if (IsAvailableAndWritable(hAcquisitionMode, "AcquisitionMode"))
    {
        err = spinEnumerationSetIntValue(hAcquisitionMode, acquisitionModeContinuous);
        if (err != SPINNAKER_ERR_SUCCESS)
        {
            printf("Unable to set acquisition mode to continuous (entry int value setting). Aborting with error %d...\n\n", err);
            goto cameraThreadClean;
        }
    }
    else
    {
        PrintRetrieveNodeFailure("entry", "AcquisitionMode");
        goto cameraThreadClean;
    }

    //
    // Begin acquiring images

    err = spinCameraBeginAcquisition(hCam);
    if (err != SPINNAKER_ERR_SUCCESS)
    {
        printf("Unable to begin image acquisition. Aborting with error %d...\n\n", err);
        goto cameraThreadClean;
    }

    g_camera_work = 1;
    for(;;)
    {
        spinImage hResultImage = NULL;

        printf("g_quit_flag =%d\n", g_quit_flag);
        if(g_quit_flag != 0)
            break;

        err = spinCameraGetNextImage(hCam, &hResultImage);
        if (err != SPINNAKER_ERR_SUCCESS)
        {
            //printf("Unable to get next image. Non-fatal error %d...\n\n", err);
            usleep(100000);
            continue;
        }

        //
        // Ensure image completion
        //
        // *** NOTES ***
        // Images can easily be checked for completion. This should be done
        // whenever a complete image is expected or required. Further, check
        // image status for a little more insight into why an image is
        // incomplete.
        //
        bool8_t isIncomplete = False;
        bool8_t hasFailed = False;

        err = spinImageIsIncomplete(hResultImage, &isIncomplete);
        if (err != SPINNAKER_ERR_SUCCESS)
        {
            //printf("Unable to determine image completion. Non-fatal error %d...\n\n", err);
            hasFailed = True;
        }

        // Check image for completion
        if (isIncomplete)
        {
            spinImageStatus imageStatus = IMAGE_NO_ERROR;

            err = spinImageGetStatus(hResultImage, &imageStatus);
            if (err != SPINNAKER_ERR_SUCCESS)
            {
                //printf("Unable to retrieve imaspinImageGetBufferSizege status. Non-fatal error %d...\n\n", imageStatus);
            }
            else
            {
                //printf("Image incomplete with image status %d...\n", imageStatus);
            }

            hasFailed = True;
        }

        // Release incomplete or failed intimage
        if (hasFailed)
        {
            err = spinImageRelease(hResultImage);
            if (err != SPINNAKER_ERR_SUCCESS)
            {
                printf("Unable to release image. Non-fatal error %d...\n\n", err);
            }

            continue;
        }

        //
        // Print image information; height and width recorded in pixels
        //
        // *** NOTES ***
        // Images have quite a bit of available metadata including things such
        // as CRC, image status, and offset values, to name a few.
        //
        size_t width = 0;
        size_t height = 0;

        //printf("Grabbed image %d, ", imageCnt);

        // Retrieve image widthint
        err = spinImageGetWidth(hResultImage, &width);
        if (err != SPINNAKER_ERR_SUCCESS)
        {
            //printf("width = unknown, ");
        }
        else
        {
            //printf("width = %u, ", (unsigned int)width);
        }

        // Retrieve image height
        err = spinImageGetHeight(hResultImage, &height);
        if (err != SPINNAKER_ERR_SUCCESS)
        {
            //printf("height = unknown\n");
        }
        else
        {
            //printf("height = %u\n", (unsigned int)height);
        }

        // Convert image to mono 8
        spinImage hConvertedImage = NULL;
        err = spinImageCreateEmpty(&hConvertedImage);
        if (err != SPINNAKER_ERR_SUCCESS)
        {
            printf("Unable to create image. Non-fatal error %d...\n\n", err);
            hasFailed = True;
        }

        err = spinImageConvert(hResultImage, PixelFormat_Mono8, hConvertedImage);
        if (err != SPINNAKER_ERR_SUCCESS)
        {
            printf("Unable to convert image. Non-fatal error %d...\n\n", err);
            hasFailed = True;
        }

        //err = spinImageSave(hConvertedImage, filename, JPEG);
        void* pimage;
        err = spinImageGetData(hConvertedImage, &pimage);
        if (err != SPINNAKER_ERR_SUCCESS)
        {
            printf("get Image data failed\n");
        }
        size_t size;
        spinImageGetBufferSize(hConvertedImage, &size);
        // printf("size = %lu\n", size);
        acq_image *image = g_images;
        for(int i=0; i< g_images_count; ++i)
        {
            if(image->ready == 1)
            {
                image ++;
                continue;
            }
            else
            {
                break;
            }
        }
        if(image->ready == 0)
        {
            image->ready = 0;
            image->height = height;
            image->width = width;
            image->type = PixelFormat_Mono8;
            image->size = size;
            memcpy(image->data, pimage, size);
            image->ready = 1;
        }

        // Destroy converted image
        err = spinImageDestroy(hConvertedImage);


        // Release image from camera
        err = spinImageRelease(hResultImage);
        if (err != SPINNAKER_ERR_SUCCESS)
        {
            printf("Unable to release image. Non-fatal error %d...\n\n", err);
        }

        usleep(40000);
    }


cameraThreadClean:
    printf("quit camera thread\n");
    if(hCam)
    {
        err = spinCameraEndAcquisition(hCam);
        if (err != SPINNAKER_ERR_SUCCESS)
        {
            printf("Unable to end acquisition. Non-fatal error %d...\n\n", err);
        }

        // Deinitialize camera
        err = spinCameraDeInit(hCam);
        if (err != SPINNAKER_ERR_SUCCESS)
        {
            printf("Unable to deinitialize camera. Aborting with error %d...\n\n", err);
        }

        // Release camera
        err = spinCameraRelease(hCam);
        if (err != SPINNAKER_ERR_SUCCESS)
        {
            //
        }
    }

    if(hCameraList)
    {
        // Clear and destroy camera list before releasing system
        err = spinCameraListClear(hCameraList);
        if (err != SPINNAKER_ERR_SUCCESS)
        {
            printf("Unable to clear camera list. Aborting with error %d...\n\n", err);
        }

        err = spinCameraListDestroy(hCameraList);
        if (err != SPINNAKER_ERR_SUCCESS)
        {
            printf("Unable to destroy camera list. Aborting with error %d...\n\n", err);
        }
    }

    if(hSystem)
    {
        // Release system
        err = spinSystemReleaseInstance(hSystem);
        if (err != SPINNAKER_ERR_SUCCESS)
        {
            printf("Unable to release system instance. Aborting with error %d...\n\n", err);
        }
    }
    printf("exit camre thread\n");
    fflush(0);
    g_camera_work = 0;
    return 0;

}


// Example entry point; please see Enumeration_C example for more in-depth
// comments on preparing and cleaning up the system.
int camera_main(/*int argc, char** argv*/)
{
    if(g_images == NULL)
        g_images = (acq_image*)malloc(sizeof(acq_image)*g_images_count);

    // Begin acquiring images
    pthread_t cli_trd;
    pthread_create(&cli_trd, NULL, camera_image_thread, (void*)0);


    return 0;
}
