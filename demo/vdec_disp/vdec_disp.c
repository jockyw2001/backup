#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <poll.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "mi_sys.h"
#include "mi_divp.h"
#include "mi_rgn.h"
#include "mi_hdmi.h"
#include "mi_disp.h"
#include "mi_vdec.h"
#include "mi_common.h"

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))
#define MI_U32VALUE(pu8Data, index) (pu8Data[index]<<24)|(pu8Data[index+1]<<16)|(pu8Data[index+2]<<8)|(pu8Data[index+3])
#define PACKET_SIZE 1024*1024

void vdec_disp(void);
void vdec_divp_disp(void);
void disp_test_001(void);

typedef struct stSysBindInfo_s
{
    MI_SYS_ChnPort_t stSrcChnPort;
    MI_SYS_ChnPort_t stDstChnPort;
    MI_U32 u32SrcFrmrate;
    MI_U32 u32DstFrmrate;
}stSysBindInfo_t;

typedef struct stVdecThreadArgs_s
{
    pthread_t pt;
    pthread_t ptsnap;
    MI_VDEC_CHN vdecChn;
    const char *szFileName;
    MI_BOOL bRunFlag;
} stVdecThreadArgs_t;

typedef struct stDispTestPubBuffThreadArgs_s
{
    pthread_t pt;
    pthread_t ptsnap;
    const char *FileName;
    MI_BOOL bRunFlag;
    MI_U16 u16BuffWidth;
    MI_U16 u16BuffHeight;
    MI_SYS_PixelFormat_e ePixelFormat;
    MI_SYS_ChnPort_t *pstSysChnPort;
}stDispTestPutBuffThreadArgs_t;

typedef void (*DISP_TEST_FUNC)(void);

typedef struct stDispTestFunc_s
{
    MI_U8 u8TestFuncIdx;
    const char *desc;
    DISP_TEST_FUNC test_func;
}stDispTestFunc_t;

stDispTestFunc_t astDispTestFunc[] = {
    {
        .u8TestFuncIdx = 0,
        .desc = "vdec-disp", 
        .test_func = vdec_disp,
    },
    {
        .u8TestFuncIdx = 1,
        .desc = "vdec-divp-disp", 
        .test_func = vdec_divp_disp,
    },
    {
        .u8TestFuncIdx = 2,
        .desc = "disp attach",
        .test_func = disp_test_001,
    },
};

typedef struct TestSubInfo_s
{
    const char *SubInfoDesc;
}TestSubInfo_t;

typedef struct TestInfo_s
{
    MI_U8 u8CaseIdx;
    const char *desc;
    const char *filePath;
    MI_U32 u32VdecOutWidth;
    MI_U32 u32VdecOutHeight;
    MI_DISP_OutputTiming_e eDispTiming;
    MI_SYS_PixelFormat_e ePixelFormat;
    MI_VDEC_CodecType_e eCodeType;
    MI_U8 u8TestSubInfoNum;
    TestSubInfo_t astTestSubInfo[10];
}TestInfo_t;

static TestInfo_t astTestInfo[] = {
    {
        .u8CaseIdx = 0,
        .desc = "vdec_disp 1x720p264",
        .filePath = "/mnt/720P25.h264",
        .u32VdecOutWidth = 1280,
        .u32VdecOutHeight = 720,
        .eDispTiming = E_MI_DISP_OUTPUT_720P60,
        .ePixelFormat = E_MI_SYS_PIXEL_FRAME_YC420_MSTTILE1_H264,
        .eCodeType = E_MI_VDEC_CODEC_TYPE_H264,
        .u8TestSubInfoNum = 3,
        .astTestSubInfo = 
        {
            {
                .SubInfoDesc = "pause",
            },
            {
                .SubInfoDesc = "resume",
            },
            {
                .SubInfoDesc = "exit",
            },
        },
    },
    {
        .u8CaseIdx = 1,
        .desc = "vdec_disp 1x1080p264",
        .filePath = "/mnt/1080P25.h264",
        .u32VdecOutWidth = 1920,
        .u32VdecOutHeight = 1080,
        .eDispTiming = E_MI_DISP_OUTPUT_1080P60,
        .ePixelFormat = E_MI_SYS_PIXEL_FRAME_YC420_MSTTILE1_H264,
        .eCodeType = E_MI_VDEC_CODEC_TYPE_H264,
        .u8TestSubInfoNum = 3,
        .astTestSubInfo = {
            {
                .SubInfoDesc = "pause",
            },
            {
                .SubInfoDesc = "resume",
            },
            {
                .SubInfoDesc = "exit",
            },
        },
    },
    {
        .u8CaseIdx = 2,
        .desc = "vdec_disp 1x4K264",
        .filePath = "/mnt/3840_2160_h264.es",
        .u32VdecOutWidth = 3840,
        .u32VdecOutHeight = 2160,
        .eDispTiming = E_MI_DISP_OUTPUT_3840x2160_60,
        .ePixelFormat = E_MI_SYS_PIXEL_FRAME_YC420_MSTTILE1_H264,
        .eCodeType = E_MI_VDEC_CODEC_TYPE_H264,
        .u8TestSubInfoNum = 3,
        .astTestSubInfo = {
            {
                .SubInfoDesc = "pause",
            },
            {
                .SubInfoDesc = "resume",
            },
            {
                .SubInfoDesc = "exit",
            },
        },
    },
    {
        .u8CaseIdx = 3,
        .desc = "vdec_disp 1x720p265",
        .filePath = "/mnt/720P25.h265",
        .u32VdecOutWidth = 1280,
        .u32VdecOutHeight = 720,
        .eDispTiming = E_MI_DISP_OUTPUT_720P60,
        .ePixelFormat = E_MI_SYS_PIXEL_FRAME_YC420_MSTTILE2_H265,
        .eCodeType = E_MI_VDEC_CODEC_TYPE_H265,
        .u8TestSubInfoNum = 3,
        .astTestSubInfo = {
            {
                .SubInfoDesc = "pause",
            },
            {
                .SubInfoDesc = "resume",
            },
            {
                .SubInfoDesc = "exit",
            },
        },
    },
    {
        .u8CaseIdx = 4,
        .desc = "vdec_disp 1x1080p265",
        .filePath = "/mnt/1080P25.h265",
        .u32VdecOutWidth = 1920,
        .u32VdecOutHeight = 1080,
        .eDispTiming = E_MI_DISP_OUTPUT_1080P60,
        .ePixelFormat = E_MI_SYS_PIXEL_FRAME_YC420_MSTTILE2_H265,
        .eCodeType = E_MI_VDEC_CODEC_TYPE_H265,
        .u8TestSubInfoNum = 3,
        .astTestSubInfo = {
            {
                .SubInfoDesc = "pause",
            },
            {
                .SubInfoDesc = "resume",
            },
            {
                .SubInfoDesc = "exit",
            },
        },
    },
    {
        .u8CaseIdx = 5,
        .desc = "vdec_disp 1x4K265",
        .filePath = "/mnt/3840_2160_h265.es",
        .u32VdecOutWidth = 3840,
        .u32VdecOutHeight = 2160,
        .eDispTiming = E_MI_DISP_OUTPUT_3840x2160_60,
        .ePixelFormat = E_MI_SYS_PIXEL_FRAME_YC420_MSTTILE2_H265,
        .eCodeType = E_MI_VDEC_CODEC_TYPE_H265,
        .u8TestSubInfoNum = 3,
        .astTestSubInfo = {
            {
                .SubInfoDesc = "pause",
            },
            {
                .SubInfoDesc = "resume",
            },
            {
                .SubInfoDesc = "exit",
            },
        },
    },
    {
        .u8CaseIdx = 6,
        .desc = "vdec_divp_disp 1x720p264",
        .filePath = "/mnt/720P25.h264",
        .u32VdecOutWidth = 1280,
        .u32VdecOutHeight = 720,
        .eDispTiming = E_MI_DISP_OUTPUT_1080P60,
        .ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV,
        .eCodeType = E_MI_VDEC_CODEC_TYPE_H264,
        .u8TestSubInfoNum = 3,
        .astTestSubInfo = {
            {
                .SubInfoDesc = "pause",
            },
            {
                .SubInfoDesc = "resume",
            },
            {
                .SubInfoDesc = "exit",
            },
        },
    },
    {
        .u8CaseIdx = 7,
        .desc = "vdec_divp_disp 1x1080p264",
        .filePath = "/mnt/1080P25.h264",
        .u32VdecOutWidth = 1920,
        .u32VdecOutHeight = 1080,
        .eDispTiming = E_MI_DISP_OUTPUT_1080P60,
        .ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV,
        .eCodeType = E_MI_VDEC_CODEC_TYPE_H264,
        .u8TestSubInfoNum = 4,
        .astTestSubInfo = {
            {
                .SubInfoDesc = "pause",
            },
            {
                .SubInfoDesc = "resume",
            },
            {
                .SubInfoDesc = "zoom",
            },
            {
                .SubInfoDesc = "exit",
            },
        },
    },
    {
        .u8CaseIdx = 8,
        .desc = "vdec_divp_disp 1x4K264",
        .filePath = "/mnt/3840_2160_h264.es",
        .u32VdecOutWidth = 3840,
        .u32VdecOutHeight = 2160,
        .eDispTiming = E_MI_DISP_OUTPUT_1080P60,
        .ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV,
        .eCodeType = E_MI_VDEC_CODEC_TYPE_H264,
        .u8TestSubInfoNum = 4,
        .astTestSubInfo = {
            {
                .SubInfoDesc = "pause",
            },
            {
                .SubInfoDesc = "resume",
            },
            {
                .SubInfoDesc = "zoom"
            },
            {
                .SubInfoDesc = "exit",
            },
        },
    },
    {
        .u8CaseIdx = 9,
        .desc = "vdec_divp_disp 1x720p265",
        .filePath = "/mnt/720P25.h265",
        .u32VdecOutWidth = 1280,
        .u32VdecOutHeight = 720,
        .eDispTiming = E_MI_DISP_OUTPUT_1080P60,
        .ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV,
        .eCodeType = E_MI_VDEC_CODEC_TYPE_H265,
        .u8TestSubInfoNum = 3,
        .astTestSubInfo = {
            {
                .SubInfoDesc = "pause",
            },
            {
                .SubInfoDesc = "resume",
            },
            {
                .SubInfoDesc = "exit",
            },
        },
    },
    {
        .u8CaseIdx = 10,
        .desc = "vdec_divp_disp 1x1080p265",
        .filePath = "/mnt/1080P25.h265",
        .u32VdecOutWidth = 1920,
        .u32VdecOutHeight = 1080,
        .eDispTiming = E_MI_DISP_OUTPUT_1080P60,
        .ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV,
        .eCodeType = E_MI_VDEC_CODEC_TYPE_H265,
        .u8TestSubInfoNum = 3,
        .astTestSubInfo = {
            {
                .SubInfoDesc = "pause",
            },
            {
                .SubInfoDesc = "resume",
            },
            {
                .SubInfoDesc = "exit",
            },
        },
    },
    {
        .u8CaseIdx = 11,
        .desc = "vdec_divp_disp 1x4K265",
        .filePath = "/mnt/3840_2160_h265.es",
        .u32VdecOutWidth = 3840,
        .u32VdecOutHeight = 2160,
        .eDispTiming = E_MI_DISP_OUTPUT_1080P60,
        .ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV,
        .eCodeType = E_MI_VDEC_CODEC_TYPE_H265,
        .u8TestSubInfoNum = 3,
        .astTestSubInfo = {
            {
                .SubInfoDesc = "pause",
            },
            {
                .SubInfoDesc = "resume",
            },
            {
                .SubInfoDesc = "exit",
            },
        },
    },
};
typedef struct disp_test001_subinfo_s
{
    const char *SubInfoDesc;
}disp_test001_subinfo_t;

typedef struct disp_test001_info_s
{
    MI_U8 u8DispTestInfoIdx;
    const char *desc;
    const char *FilePath;
    MI_DISP_OutputTiming_e edev0Timing;
    MI_DISP_OutputTiming_e edev1Timing;
    MI_HDMI_TimingType_e eHdmiTiming;
    MI_SYS_PixelFormat_e ePixelFormat;
    MI_U16 u16LayerWidth;
    MI_U16 u16LayerHeight;
    MI_U16 u16InputWidth;
    MI_U16 u16InputHeight;
    MI_U8 u8TestSubInfoNum;
    disp_test001_subinfo_t astDispTest001SubInfo[10];
}disp_test001_info_t;

static disp_test001_info_t astDispTest001Info[] = {
    {
        .u8DispTestInfoIdx = 0,
        .desc = "dev0 720p60,dev1 720p60",
        .FilePath = "/mnt/YUV422_YUYV1280_720.yuv",
        .edev0Timing = E_MI_DISP_OUTPUT_720P60,
        .edev1Timing = E_MI_DISP_OUTPUT_720P60,
        .eHdmiTiming = E_MI_HDMI_TIMING_720_60P,
        .ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV,
        .u16LayerWidth = 1280,
        .u16LayerHeight = 720,
        .u16InputWidth = 1280,
        .u16InputHeight = 720,
        .u8TestSubInfoNum = 3,
        .astDispTest001SubInfo = {
            {
                .SubInfoDesc = "pause",
            },
            {
                .SubInfoDesc = "resume",
            },
            {
                .SubInfoDesc = "exit",
            },
        },
    },
    {
        .u8DispTestInfoIdx = 1,
        .desc = "dev0 1080p60,dev1 720p60",
        .FilePath = "/mnt/YUV422_YUYV1920_1080.yuv",
        .edev0Timing = E_MI_DISP_OUTPUT_1080P60,
        .edev1Timing = E_MI_DISP_OUTPUT_720P60,
        .eHdmiTiming = E_MI_HDMI_TIMING_1080_60P,
        .ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV,
        .u16LayerWidth = 1920,
        .u16LayerHeight = 1080,
        .u16InputWidth = 1920,
        .u16InputHeight = 1080,
        .u8TestSubInfoNum = 3,
        .astDispTest001SubInfo = {
            {
                .SubInfoDesc = "pause",
            },
            {
                .SubInfoDesc = "resume",
            },
            {
                .SubInfoDesc = "exit",
            },
        },
    },
    {
        .u8DispTestInfoIdx = 2,
        .desc = "dev0 1080p60,dev1 1080p60",
        .FilePath = "/mnt/YUV422_YUYV1920_1080.yuv",
        .edev0Timing = E_MI_DISP_OUTPUT_1080P60,
        .edev1Timing = E_MI_DISP_OUTPUT_1080P60,
        .eHdmiTiming = E_MI_HDMI_TIMING_1080_60P,
        .ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV,
        .u16LayerWidth = 1920,
        .u16LayerHeight = 1080,
        .u16InputWidth = 1920,
        .u16InputHeight = 1080,
        .u8TestSubInfoNum = 3,
        .astDispTest001SubInfo = {
            {
                .SubInfoDesc = "pause",
            },
            {
                .SubInfoDesc = "resume",
            },
            {
                .SubInfoDesc = "exit",
            },
        },
    },
};

static MI_BOOL g_PushDataExit = FALSE;
static MI_BOOL g_PushDataStop = FALSE;
stVdecThreadArgs_t gstVdecThread;
stDispTestPutBuffThreadArgs_t gstDispTestPutBufThread;

static int Get_Test_Num(void)
{
    char buffer[256];
    memset(buffer, 0, sizeof(buffer));
    printf("please input test num\n");
    fgets(buffer, sizeof(buffer), stdin);
    return atoi(buffer);
}

MI_U64 Sys_GetPts(MI_U32 u32FrameRate)
{
    if (0 == u32FrameRate)
    {
        return (MI_U64)(-1);
    }
    return (MI_U64)(1000 / u32FrameRate);
}

MI_S32 Sys_Init(void)
{
    MI_SYS_Version_t stVersion;
    MI_U64 u64Pts = 0;
    MI_SYS_Init(); //Sys Init
    memset(&stVersion, 0x0, sizeof(MI_SYS_Version_t));
    MI_SYS_GetVersion(&stVersion);
    MI_SYS_GetCurPts(&u64Pts);
    u64Pts = 0xF1237890F1237890;
    MI_SYS_InitPtsBase(u64Pts);

    u64Pts = 0xE1237890E1237890;
    MI_SYS_SyncPts(u64Pts);

    return MI_SUCCESS;
}

MI_S32 Sys_Exit(void)
{
    MI_SYS_Exit();
    return MI_SUCCESS;
}

MI_S32 Sys_Bind(stSysBindInfo_t *pstBindInfo)
{
    MI_SYS_BindChnPort(&pstBindInfo->stSrcChnPort, &pstBindInfo->stDstChnPort, \
        pstBindInfo->u32SrcFrmrate, pstBindInfo->u32DstFrmrate);

    return MI_SUCCESS;
}

MI_S32 Sys_UnBind(stSysBindInfo_t *pstBindInfo)
{
    MI_SYS_UnBindChnPort(&pstBindInfo->stSrcChnPort, &pstBindInfo->stDstChnPort);

    return MI_SUCCESS;
}

static MI_S32 Hdmi_Start(MI_HDMI_DeviceId_e eHdmi, MI_HDMI_TimingType_e eTimingType)
{
    MI_HDMI_Attr_t stAttr;
    memset(&stAttr, 0, sizeof(MI_HDMI_Attr_t));
    stAttr.stEnInfoFrame.bEnableAudInfoFrame  = FALSE;
    stAttr.stEnInfoFrame.bEnableAviInfoFrame  = FALSE;
    stAttr.stEnInfoFrame.bEnableSpdInfoFrame  = FALSE;
    stAttr.stAudioAttr.bEnableAudio = TRUE;
    stAttr.stAudioAttr.bIsMultiChannel = 0;
    stAttr.stAudioAttr.eBitDepth = E_MI_HDMI_BIT_DEPTH_16;
    stAttr.stAudioAttr.eCodeType = E_MI_HDMI_ACODE_PCM;
    stAttr.stAudioAttr.eSampleRate = E_MI_HDMI_AUDIO_SAMPLERATE_48K;
    stAttr.stVideoAttr.bEnableVideo = TRUE;
    stAttr.stVideoAttr.eColorType = E_MI_HDMI_COLOR_TYPE_YCBCR444;//default color type
    stAttr.stVideoAttr.eDeepColorMode = E_MI_HDMI_DEEP_COLOR_MAX;
    stAttr.stVideoAttr.eTimingType = eTimingType;
    stAttr.stVideoAttr.eOutputMode = E_MI_HDMI_OUTPUT_MODE_HDMI;
    MI_HDMI_SetAttr(eHdmi, &stAttr);
    MI_HDMI_Start(eHdmi);
    
    return MI_SUCCESS;
}

static MI_S32 Hdmi_callback(MI_HDMI_DeviceId_e eHdmi, MI_HDMI_EventType_e Event, void *pEventParam, void *pUsrParam)
{
    switch (Event)
    {
        case E_MI_HDMI_EVENT_HOTPLUG:
            printf("E_MI_HDMI_EVENT_HOTPLUG.\n");
            MI_HDMI_Start(eHdmi);
            break;
        case E_MI_HDMI_EVENT_NO_PLUG:
            printf("E_MI_HDMI_EVENT_NO_PLUG.\n");
            MI_HDMI_Stop(eHdmi);
            break;
        default:
            printf("Unsupport event.\n");
            break;
    }

    return MI_SUCCESS;
}

MI_BOOL disp_test_OpenSourceFile(const char *pFileName, int *pSrcFd)
{
    int src_fd = open(pFileName, O_RDONLY);
    if (src_fd < 0)
    {
        printf("src_file: %s.\n", pFileName);

        perror("open");
        return -1;
    }
    *pSrcFd = src_fd;

    return TRUE;
}

MI_S32 disp_test_GetOneFrame(int srcFd, int offset, char *pData, int yuvSize)
{
    if (read(srcFd, pData, yuvSize) < yuvSize)
    {
        lseek(srcFd, 0, SEEK_SET);
        if (read(srcFd, pData, yuvSize) < yuvSize)
        {
            printf(" [%s %d] read file error.\n", __FUNCTION__, __LINE__);
            return FALSE;
        }
    }
    return TRUE;
}


static void* VdecSendStream(void *args)
{
    MI_SYS_ChnPort_t stChnPort;
    MI_SYS_BufConf_t stBufConf;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hSysBuf;
    MI_S32 s32Ret = MI_SUCCESS;
    MI_VDEC_CHN vdecChn = 0;
    MI_S32 s32TimeOutMs = 20, s32ChannelId = 0, s32TempHeight = 0;
    MI_S32 s32Ms = 30;
    MI_BOOL bVdecChnEnable;
    MI_U16 u16Times = 20000;

    MI_S32 readfd = 0;
    MI_U8 *pu8Buf = NULL;
    MI_S32 s32Len = 0;
    MI_U32 u32FrameLen = 0;
    MI_U64 u64Pts = 0;
    MI_U8 au8Header[16] = {0};
    MI_U32 u32Pos = 0;
    MI_VDEC_ChnStat_t stChnStat;
    MI_VDEC_VideoStream_t stVdecStream;
    MI_VDEC_ChnStat_t stVdecStat;

    stVdecThreadArgs_t *pstArgs = (stVdecThreadArgs_t *)args;

    char tname[32];
    memset(tname, 0, 32);

    vdecChn = pstArgs->vdecChn;
    snprintf(tname, 32, "push_t_%u", vdecChn);
    prctl(PR_SET_NAME, tname);

    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_VDEC;
    stChnPort.u32DevId = 0;
    stChnPort.u32ChnId = vdecChn;//0 1 2 3
    stChnPort.u32PortId = 0;

    readfd = open(pstArgs->szFileName,  O_RDONLY, 0); //ES
    if (0 > readfd)
    {
        printf("Open %s failed!\n", pstArgs->szFileName);
        return NULL;
    }

    memset(&stBufConf, 0x0, sizeof(MI_SYS_BufConf_t));
    stBufConf.eBufType = E_MI_SYS_BUFDATA_RAW;
    stBufConf.u64TargetPts = 0;
    pu8Buf = malloc(PACKET_SIZE);

    //MI_SYS_SetChnOutputPortDepth(&stChnPort, 2, 5);

    s32Ms = 40;
    printf("Start----------------------%d------------------Start\n", stChnPort.u32ChnId);
    while (!g_PushDataExit)
    {
        usleep(s32Ms * 1000);//33
        if(g_PushDataStop == false)
        {
            if (pstArgs->bRunFlag == FALSE)
            {
                continue;
            }
            memset(au8Header, 0, 16);
            u32Pos = lseek(readfd, 0L, SEEK_CUR);
            s32Len = read(readfd, au8Header, 16);
            if(s32Len <= 0)
            {
                lseek(readfd, 0, SEEK_SET);
                continue;
            }
            u32FrameLen = MI_U32VALUE(au8Header, 4);
            //printf("vdecChn:%d, u32FrameLen:%d\n", vdecChn, u32FrameLen);
            if(u32FrameLen > PACKET_SIZE)
            {
                lseek(readfd, 0, SEEK_SET);
                continue;
            }
            s32Len = read(readfd, pu8Buf, u32FrameLen);
            if(s32Len <= 0)
            {
                lseek(readfd, 0, SEEK_SET);
                continue;
            }

            memset(&stVdecStat, 0, sizeof(MI_VDEC_ChnStat_t));
            if (MI_SUCCESS == (s32Ret = MI_VDEC_GetChnStat(vdecChn, &stVdecStat)))
            {
                if (stVdecStat.u32LeftStreamFrames > 3)
                    printf("vdecChn:%d,u32RecvStreamFrames:%d,u32LeftStreamBytes:%d,u32LeftStreamFrames:%d\n", vdecChn,
                        stVdecStat.u32RecvStreamFrames, stVdecStat.u32LeftStreamBytes, stVdecStat.u32LeftStreamFrames);
            }
            else
            {
                printf("vdec:%d, MI_VDEC_GetChnStat Error, 0x%X\n", vdecChn, s32Ret);
            }

            stVdecStream.pu8Addr = pu8Buf;
            stVdecStream.u32Len = s32Len;
            stVdecStream.u64PTS = u64Pts;
            stVdecStream.bEndOfFrame = 1;
            stVdecStream.bEndOfStream = 0;
            if (MI_SUCCESS != (s32Ret = MI_VDEC_SendStream(vdecChn, &stVdecStream, s32TimeOutMs)))
            {
                printf("MI_VDEC_SendStream fail, chn:%d, 0x%X\n", vdecChn, s32Ret);
            }
            u64Pts = u64Pts + Sys_GetPts(30);
            //memset(&stChnStat, 0x0, sizeof(stChnStat));
            //MI_VDEC_GetChnStat(s32VoChannel, &stChnStat);
        }
        else
        {
            MI_DISP_ClearInputPortBuffer(0 ,0, 1);
        }
    }
    printf("\n\n");
    usleep(300000);
    free(pu8Buf);
    close(readfd);

    printf("End----------------------%d------------------End\n", stChnPort.u32ChnId);

    return NULL;
}

static void *disp_test_PutBuffer (void *pData)
{
    int srcFd = 0;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BufConf_t stBufConf;
    MI_U32 u32BuffSize;
    MI_SYS_BUF_HANDLE hHandle;
    struct timeval stTv;
    struct timeval stGetBuffer, stReadFile, stFlushData, stPutBuffer, stRelease;
    time_t stTime = 0;
    stDispTestPutBuffThreadArgs_t *pstDispTestPutBufArgs = (stDispTestPutBuffThreadArgs_t *)pData;
    MI_SYS_ChnPort_t *pstSysChnPort = pstDispTestPutBufArgs->pstSysChnPort;
    const char *FilePath = pstDispTestPutBufArgs->FileName;

    memset(&stBufInfo , 0 , sizeof(MI_SYS_BufInfo_t));
    memset(&stBufConf , 0 , sizeof(MI_SYS_BufConf_t));
    memset(&stTv, 0, sizeof(stTv));
    
    printf("----------------------start----------------------\n");
    if (TRUE == disp_test_OpenSourceFile(FilePath, &srcFd))
    {
        while(!g_PushDataExit)
        {
            if(g_PushDataStop == FALSE)
            {
                gettimeofday(&stTv, NULL);
                stBufConf.eBufType = E_MI_SYS_BUFDATA_FRAME;
                stBufConf.u64TargetPts = stTv.tv_sec*1000000 + stTv.tv_usec;
                stBufConf.stFrameCfg.u16Width = pstDispTestPutBufArgs->u16BuffWidth;
                stBufConf.stFrameCfg.u16Height = pstDispTestPutBufArgs->u16BuffHeight;
                stBufConf.stFrameCfg.eFormat = pstDispTestPutBufArgs->ePixelFormat;
                stBufConf.stFrameCfg.eFrameScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
                gettimeofday(&stGetBuffer, NULL);

                if(MI_SUCCESS == MI_SYS_ChnInputPortGetBuf(pstSysChnPort,&stBufConf,&stBufInfo,&hHandle, -1))
                {
                    stBufInfo.stFrameData.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
                    stBufInfo.stFrameData.eFieldType = E_MI_SYS_FIELDTYPE_NONE;
                    stBufInfo.stFrameData.eTileMode = E_MI_SYS_FRAME_TILE_MODE_NONE;
                    stBufInfo.bEndOfStream = FALSE;

                    u32BuffSize = stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0];
                    gettimeofday(&stReadFile, NULL);

                    if(disp_test_GetOneFrame(srcFd, 0, stBufInfo.stFrameData.pVirAddr[0], u32BuffSize) == TRUE)
                    {
                        gettimeofday(&stFlushData, NULL);
                        MI_SYS_FlushInvCache(stBufInfo.stFrameData.pVirAddr[0],u32BuffSize);
                        gettimeofday(&stPutBuffer, NULL);

                        MI_SYS_ChnInputPortPutBuf(hHandle ,&stBufInfo , FALSE);
                        gettimeofday(&stRelease, NULL);
                    }
                    else
                    {
                        printf("disp_test_001 getframe failed\n");
                        MI_SYS_ChnInputPortPutBuf(hHandle ,&stBufInfo , TRUE);
                    }

               }
               else
               {
                    printf("disp_test_001 sys get buf fail\n");
               }
               usleep(100*1000);
            }
            
        }
        close(srcFd);
    }
    else
    {
        printf(" open file fail. \n");
    }
    printf("----------------------end----------------------\n");

    return MI_DISP_SUCCESS;
}

void disp_test_001(void)
{
    MI_U8 n = 0;
    pthread_t thread;
    MI_U8 u8TestNum = 0;
    MI_U8 u8TestSubNum = 0;
    MI_DISP_DEV DispDev0 = 0;
    MI_DISP_DEV DispDev1 = 1;
    MI_DISP_LAYER DispLayer0 = 0;
    MI_DISP_LAYER DispLayer1 = 1;
    MI_DISP_INPUTPORT InputPort = 0;
    MI_DISP_PubAttr_t stPubAttr;
    MI_DISP_VideoLayerAttr_t stLayerAttr;
    MI_DISP_InputPortAttr_t stInputPortAttr;
    MI_HDMI_InitParam_t stInitParam;
    MI_HDMI_DeviceId_e eHdmi = E_MI_HDMI_ID_0;
    MI_SYS_PixelFormat_e ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
    MI_SYS_ChnPort_t stSysChnPort;

    for(n = 0; n < (sizeof(astDispTest001Info)/sizeof(disp_test001_info_t)); n++)
    {
        printf("[%d]%s\n",n,astDispTest001Info[n].desc);
    }
    u8TestNum = Get_Test_Num();

    stSysChnPort.eModId = E_MI_MODULE_ID_DISP;
    stSysChnPort.u32DevId = DispDev0;
    stSysChnPort.u32ChnId = 0;
    stSysChnPort.u32PortId = 0;

    //set hdmi
    stInitParam.pCallBackArgs = NULL;
    stInitParam.pfnHdmiEventCallback = Hdmi_callback;
    MI_HDMI_Init(&stInitParam);
    MI_HDMI_Open(eHdmi);
    
    //set disp0 pub
    memset(&stPubAttr, 0, sizeof(stPubAttr));
    stPubAttr.eIntfSync = astDispTest001Info[u8TestNum].edev0Timing;
    stPubAttr.eIntfType = E_MI_DISP_INTF_HDMI;
    MI_DISP_SetPubAttr(DispDev0,  &stPubAttr);
    MI_DISP_Enable(DispDev0);
    
    //set disp1 pub
    memset(&stPubAttr, 0, sizeof(stPubAttr));
    stPubAttr.eIntfSync = astDispTest001Info[u8TestNum].edev1Timing;
    stPubAttr.eIntfType = E_MI_DISP_INTF_VGA;
    MI_DISP_SetPubAttr(DispDev1,&stPubAttr);
    MI_DISP_Enable(DispDev1);
    
    memset(&stLayerAttr, 0, sizeof(stLayerAttr));
    stLayerAttr.stVidLayerSize.u16Width = astDispTest001Info[u8TestNum].u16LayerWidth;
    stLayerAttr.stVidLayerSize.u16Height= astDispTest001Info[u8TestNum].u16LayerHeight;
    stLayerAttr.stVidLayerDispWin.u16X = 0;
    stLayerAttr.stVidLayerDispWin.u16Y = 0;
    stLayerAttr.stVidLayerDispWin.u16Width = astDispTest001Info[u8TestNum].u16LayerWidth;
    stLayerAttr.stVidLayerDispWin.u16Height = astDispTest001Info[u8TestNum].u16LayerHeight;
    stLayerAttr.ePixFormat = ePixelFormat;

    MI_DISP_BindVideoLayer(DispLayer0,DispDev0);
    MI_DISP_SetVideoLayerAttr(DispLayer0, &stLayerAttr);
    MI_DISP_GetVideoLayerAttr(DispLayer0, &stLayerAttr);
    MI_DISP_EnableVideoLayer(DispLayer0);

    memset(&stInputPortAttr, 0, sizeof(stInputPortAttr));

    MI_DISP_GetInputPortAttr(DispLayer0, InputPort, &stInputPortAttr);
    stInputPortAttr.stDispWin.u16Width = astDispTest001Info[u8TestNum].u16InputWidth;
    stInputPortAttr.stDispWin.u16Height = astDispTest001Info[u8TestNum].u16InputHeight;
    MI_DISP_SetInputPortAttr(DispLayer0, InputPort, &stInputPortAttr);
    MI_DISP_EnableInputPort(DispLayer0, InputPort);

    Hdmi_Start(eHdmi,astDispTest001Info[u8TestNum].eHdmiTiming);

    MI_DISP_DeviceAttach(DispDev0, DispDev1);
    
    gstDispTestPutBufThread.FileName = astDispTest001Info[u8TestNum].FilePath;
    gstDispTestPutBufThread.bRunFlag = TRUE;
    gstDispTestPutBufThread.u16BuffWidth = astDispTest001Info[u8TestNum].u16InputWidth;
    gstDispTestPutBufThread.u16BuffHeight = astDispTest001Info[u8TestNum].u16InputHeight;
    gstDispTestPutBufThread.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
    gstDispTestPutBufThread.pstSysChnPort = &stSysChnPort;
    
    pthread_create(&gstDispTestPutBufThread.pt, NULL, disp_test_PutBuffer, &gstDispTestPutBufThread);

    while(1)
    {
        for(n = 0; n < astDispTest001Info[u8TestNum].u8TestSubInfoNum; n++)
        {
            printf("[%d]%s\n",n,astDispTest001Info[u8TestNum].astDispTest001SubInfo[n].SubInfoDesc);
        }
        u8TestSubNum = Get_Test_Num();           
        if(0 == strncmp(astDispTest001Info[u8TestNum].astDispTest001SubInfo[u8TestSubNum].SubInfoDesc,"pause",5))
        {
            g_PushDataStop = TRUE;
        }
        if(0 == strncmp(astDispTest001Info[u8TestNum].astDispTest001SubInfo[u8TestSubNum].SubInfoDesc,"resume",6))
        {
            g_PushDataStop = FALSE;
        }
        if(0 == strncmp(astDispTest001Info[u8TestNum].astDispTest001SubInfo[u8TestSubNum].SubInfoDesc,"exit",4))
        {
            g_PushDataExit = TRUE;
            pthread_join(gstDispTestPutBufThread.pt,NULL);
            gstDispTestPutBufThread.pt = 0;

            MI_DISP_DisableInputPort(DispLayer0,InputPort);
            MI_DISP_DisableVideoLayer(DispLayer0);
            MI_DISP_UnBindVideoLayer(DispLayer0,DispDev0);
            MI_DISP_Disable(DispDev0);
            MI_DISP_Disable(DispDev1);
            break;
        }
    }
}

void vdec_disp(void)
{
    MI_U8 n = 0;
    MI_U8 u8TestNum;
    MI_U8 u8TestSubNum;
    MI_U32 u32Width;
    MI_U32 u32Height;
    MI_HDMI_InitParam_t stInitParam;
    MI_VDEC_ChnAttr_t stVdecChnAttr;
    MI_DISP_PubAttr_t stPubAttr;
    MI_DISP_VideoLayerAttr_t stLayerAttr;
    MI_DISP_InputPortAttr_t stInputPortAttr;
    stSysBindInfo_t stBindInfo;
    MI_HDMI_DeviceId_e eHdmi = E_MI_HDMI_ID_0;
    MI_DISP_DEV DispDev = 0;
    MI_DISP_LAYER DispLayer = 0;
    const char* filePath = NULL;
    MI_SYS_PixelFormat_e ePixelFormat;
    MI_VDEC_CodecType_e eCodeType;
    MI_DISP_OutputTiming_e eDispTiming;
    MI_HDMI_TimingType_e eHdmiTiming;

    for(n = 0; n < (sizeof(astTestInfo)/sizeof(TestInfo_t)); n++)
    {
        printf("[%d]%s\n",n,astTestInfo[n].desc);
    }
    u8TestNum = Get_Test_Num();

    filePath = astTestInfo[u8TestNum].filePath;
    eDispTiming = astTestInfo[u8TestNum].eDispTiming;
    ePixelFormat = astTestInfo[u8TestNum].ePixelFormat;
    eCodeType = astTestInfo[u8TestNum].eCodeType;
    
    switch(eDispTiming)
    {
        case E_MI_DISP_OUTPUT_720P60:
            eHdmiTiming = E_MI_HDMI_TIMING_720_60P;
            u32Width = 1280;
            u32Height = 720;
            break;
        case E_MI_DISP_OUTPUT_1080P60:
            eHdmiTiming = E_MI_HDMI_TIMING_1080_60P;
            u32Width = 1920;
            u32Height = 1080;           
            break;
        case E_MI_DISP_OUTPUT_3840x2160_60:
            eHdmiTiming = E_MI_HDMI_TIMING_4K2K_60P;
            u32Width = 3840;
            u32Height = 2160;
            break;
        default:
            eHdmiTiming = E_MI_HDMI_TIMING_1080_60P;
            u32Width = 1920;
            u32Height = 1080;           
            break;          
    }
   
    stInitParam.pCallBackArgs = NULL;
    stInitParam.pfnHdmiEventCallback = Hdmi_callback;
    MI_HDMI_Init(&stInitParam);
    MI_HDMI_Open(eHdmi);

    memset(&stVdecChnAttr, 0, sizeof(MI_VDEC_ChnAttr_t));
    stVdecChnAttr.stVdecVideoAttr.u32RefFrameNum = 2;
    stVdecChnAttr.eVideoMode    = E_MI_VDEC_VIDEO_MODE_FRAME;
    stVdecChnAttr.u32PicWidth   = u32Width;
    stVdecChnAttr.u32BufSize    = 4 * 1024 * 1024;
    stVdecChnAttr.u32PicHeight  = u32Height;
    stVdecChnAttr.u32Priority   = 0;
    stVdecChnAttr.eCodecType    = eCodeType;

    MI_VDEC_CreateChn(0, &stVdecChnAttr);
    MI_VDEC_StartChn(0);

    memset(&stPubAttr, 0, sizeof(stPubAttr));
    stPubAttr.eIntfSync = eDispTiming;
    stPubAttr.eIntfType = E_MI_DISP_INTF_HDMI;
    MI_DISP_SetPubAttr(DispDev, &stPubAttr);
    MI_DISP_Enable(DispDev);

    memset(&stLayerAttr, 0, sizeof(stLayerAttr));
    MI_DISP_BindVideoLayer(DispLayer, DispDev);
    stLayerAttr.ePixFormat = ePixelFormat;
    stLayerAttr.stVidLayerSize.u16Width  = u32Width;
    stLayerAttr.stVidLayerSize.u16Height = u32Height;
    stLayerAttr.stVidLayerDispWin.u16X      = 0;
    stLayerAttr.stVidLayerDispWin.u16Y      = 0;
    stLayerAttr.stVidLayerDispWin.u16Width  = u32Width;
    stLayerAttr.stVidLayerDispWin.u16Height = u32Height;
    MI_DISP_SetVideoLayerAttr(DispLayer, &stLayerAttr);
    MI_DISP_EnableVideoLayer(DispLayer);

    memset(&stInputPortAttr, 0, sizeof(stInputPortAttr));
    stInputPortAttr.stDispWin.u16X      = 0;
    stInputPortAttr.stDispWin.u16Y      = 0;
    stInputPortAttr.stDispWin.u16Width  = u32Width;
    stInputPortAttr.stDispWin.u16Height = u32Height;
    MI_DISP_SetInputPortAttr(DispLayer, 0, &stInputPortAttr);
    MI_DISP_GetInputPortAttr(DispLayer, 0, &stInputPortAttr);
    MI_DISP_EnableInputPort(DispLayer, 0);
    MI_DISP_SetInputPortSyncMode (DispLayer, 0, E_MI_DISP_SYNC_MODE_FREE_RUN);
    
    Hdmi_Start(eHdmi,eHdmiTiming);

    memset(&stBindInfo, 0, sizeof(stSysBindInfo_t));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VDEC;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = 0;
    stBindInfo.stSrcChnPort.u32PortId = 0;

    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DISP;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = 0;
    stBindInfo.stDstChnPort.u32PortId = 0;
    stBindInfo.u32SrcFrmrate = 0;
    stBindInfo.u32DstFrmrate = 0;
    Sys_Bind(&stBindInfo);

    gstVdecThread.bRunFlag = TRUE;
    gstVdecThread.vdecChn = 0;
    gstVdecThread.szFileName = filePath;
    pthread_create(&gstVdecThread.pt, NULL, VdecSendStream, (void *)&gstVdecThread);

    while(1)
    {
        for(n = 0; n < astTestInfo[u8TestNum].u8TestSubInfoNum; n++)
        {
            printf("[%d]%s\n",n,astTestInfo[u8TestNum].astTestSubInfo[n].SubInfoDesc);
        }
        u8TestSubNum = Get_Test_Num();

        if(0 == strncmp(astTestInfo[u8TestNum].astTestSubInfo[u8TestSubNum].SubInfoDesc,"pause",5))
        {
            g_PushDataStop = TRUE;    
        }
        else if(0 == strncmp(astTestInfo[u8TestNum].astTestSubInfo[u8TestSubNum].SubInfoDesc,"resume",6))
        {
            g_PushDataStop = FALSE;
        }
        else if(0 == strncmp(astTestInfo[u8TestNum].astTestSubInfo[u8TestSubNum].SubInfoDesc,"exit",4))
        {
            g_PushDataExit = TRUE;
            pthread_join(gstVdecThread.pt, NULL);
            gstVdecThread.pt = 0;

            memset(&stBindInfo, 0, sizeof(stSysBindInfo_t));
            stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VDEC;
            stBindInfo.stSrcChnPort.u32DevId = 0;
            stBindInfo.stSrcChnPort.u32ChnId = 0;
            stBindInfo.stSrcChnPort.u32PortId = 0;

            stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DISP;
            stBindInfo.stDstChnPort.u32DevId = 0;
            stBindInfo.stDstChnPort.u32ChnId = 0;
            stBindInfo.stDstChnPort.u32PortId = 0;
            Sys_UnBind(&stBindInfo);

            MI_VDEC_StopChn(0);
            MI_VDEC_DestroyChn(0);
 
            MI_DISP_DisableInputPort(DispLayer, 0);
            MI_DISP_DisableVideoLayer(DispLayer);
            MI_DISP_UnBindVideoLayer(DispLayer, DispDev);
            MI_DISP_Disable(DispDev);

            MI_HDMI_Stop(0);
            MI_HDMI_Close(0);
            MI_HDMI_DeInit();
            break;
        }
    }
}

void vdec_divp_disp(void)
{
    MI_U8 n = 0;
    MI_U8 u8TestNum;
    MI_U8 u8TestSubNum;
    MI_U32 u32Width;
    MI_U32 u32Height;
    MI_HDMI_InitParam_t stInitParam;
    MI_VDEC_ChnAttr_t stVdecChnAttr;
    MI_DIVP_ChnAttr_t stDivpChnAttr;
    MI_DIVP_OutputPortAttr_t stOutputPortAttr;
    MI_DISP_PubAttr_t stPubAttr;
    MI_DISP_VideoLayerAttr_t stLayerAttr;
    MI_DISP_InputPortAttr_t stInputPortAttr;
    stSysBindInfo_t stBindInfo;
    MI_HDMI_DeviceId_e eHdmi = E_MI_HDMI_ID_0;
    MI_DISP_DEV DispDev = 0;
    MI_DISP_LAYER DispLayer = 0;
    char* filePath = NULL;
    MI_DISP_OutputTiming_e eDispTiming;
    MI_HDMI_TimingType_e eHdmiTiming;
    MI_SYS_PixelFormat_e ePixelFormat;
    MI_VDEC_CodecType_e eCodeType;
    MI_U32 u32VdecOutWidth;
    MI_U32 u32VdecOutHeight;

    for(n = 0; n < (sizeof(astTestInfo)/sizeof(TestInfo_t)); n++)
    {
        printf("[%d]%s\n",n,astTestInfo[n].desc);
    }
    u8TestNum = Get_Test_Num();

    filePath = astTestInfo[u8TestNum].filePath;
    u32VdecOutWidth = astTestInfo[u8TestNum].u32VdecOutWidth;
    u32VdecOutHeight = astTestInfo[u8TestNum].u32VdecOutHeight;
    eDispTiming = astTestInfo[u8TestNum].eDispTiming;
    ePixelFormat = astTestInfo[u8TestNum].ePixelFormat;
    eCodeType = astTestInfo[u8TestNum].eCodeType;
    
    switch(eDispTiming)
    {
        case E_MI_DISP_OUTPUT_720P60:
            eHdmiTiming = E_MI_HDMI_TIMING_720_60P;
            u32Width = 1280;
            u32Height = 720;
            break;
        case E_MI_DISP_OUTPUT_1080P60:
            eHdmiTiming = E_MI_HDMI_TIMING_1080_60P;
            u32Width = 1920;
            u32Height = 1080;           
            break;
        case E_MI_DISP_OUTPUT_3840x2160_60:
            eHdmiTiming = E_MI_HDMI_TIMING_4K2K_60P;
            u32Width = 3840;
            u32Height = 2160;
            break;
        default:
            eHdmiTiming = E_MI_HDMI_TIMING_1080_60P;
            u32Width = 1920;
            u32Height = 1080;           
            break;          
    }

    //set hdmi  
    stInitParam.pCallBackArgs = NULL;
    stInitParam.pfnHdmiEventCallback = Hdmi_callback;
    MI_HDMI_Init(&stInitParam);
    MI_HDMI_Open(eHdmi);

    //set vdec
    memset(&stVdecChnAttr, 0, sizeof(MI_VDEC_ChnAttr_t));
    stVdecChnAttr.stVdecVideoAttr.u32RefFrameNum = 2;
    stVdecChnAttr.eVideoMode    = E_MI_VDEC_VIDEO_MODE_FRAME;
    stVdecChnAttr.u32PicWidth   = u32VdecOutWidth;
    stVdecChnAttr.u32BufSize    = 4 * 1024 * 1024;
    stVdecChnAttr.u32PicHeight  = u32VdecOutHeight;
    stVdecChnAttr.u32Priority   = 0;
    stVdecChnAttr.eCodecType    = eCodeType;

    MI_VDEC_CreateChn(0, &stVdecChnAttr);
    MI_VDEC_StartChn(0);

    //set divp
    memset(&stDivpChnAttr, 0, sizeof(MI_DIVP_ChnAttr_t));
    stDivpChnAttr.bHorMirror            = FALSE;
    stDivpChnAttr.bVerMirror            = FALSE;
    stDivpChnAttr.eDiType               = E_MI_DIVP_DI_TYPE_OFF;
    stDivpChnAttr.eRotateType           = E_MI_SYS_ROTATE_NONE;
    stDivpChnAttr.eTnrLevel             = E_MI_DIVP_TNR_LEVEL_OFF;
    stDivpChnAttr.stCropRect.u16X       = 0; 
    stDivpChnAttr.stCropRect.u16Y       = 0; 
    stDivpChnAttr.stCropRect.u16Width   = u32VdecOutWidth; //Vdec pic w
    stDivpChnAttr.stCropRect.u16Height  = u32VdecOutHeight; //Vdec pic h
    stDivpChnAttr.u32MaxWidth           = u32VdecOutWidth; //max size picture can pass
    stDivpChnAttr.u32MaxHeight          = u32VdecOutHeight;
    MI_DIVP_CreateChn(0, &stDivpChnAttr);
    MI_DIVP_StartChn(0);

    memset(&stOutputPortAttr, 0, sizeof(MI_DIVP_OutputPortAttr_t));
    stOutputPortAttr.eCompMode          = E_MI_SYS_COMPRESS_MODE_NONE;
    stOutputPortAttr.ePixelFormat       = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
    stOutputPortAttr.u32Width           = u32Width;
    stOutputPortAttr.u32Height          = u32Height;
    MI_DIVP_SetOutputPortAttr(0, &stOutputPortAttr);
 
    //bind vdec - divp
    memset(&stBindInfo, 0, sizeof(stSysBindInfo_t));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VDEC;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = 0;
    stBindInfo.stSrcChnPort.u32PortId = 0;

    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DIVP;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = 0;
    stBindInfo.stDstChnPort.u32PortId = 0;
    stBindInfo.u32SrcFrmrate = 0;
    stBindInfo.u32DstFrmrate = 0;
    Sys_Bind(&stBindInfo);
  
    //set disp
    memset(&stPubAttr, 0, sizeof(stPubAttr));
    stPubAttr.eIntfSync = eDispTiming;
    stPubAttr.eIntfType = E_MI_DISP_INTF_HDMI;
    MI_DISP_SetPubAttr(DispDev, &stPubAttr);
    MI_DISP_Enable(DispDev);

    memset(&stLayerAttr, 0, sizeof(stLayerAttr));
    MI_DISP_BindVideoLayer(DispLayer, DispDev);
    stLayerAttr.ePixFormat = ePixelFormat;
    stLayerAttr.stVidLayerSize.u16Width  = u32Width;
    stLayerAttr.stVidLayerSize.u16Height = u32Height;
    stLayerAttr.stVidLayerDispWin.u16X      = 0;
    stLayerAttr.stVidLayerDispWin.u16Y      = 0;
    stLayerAttr.stVidLayerDispWin.u16Width  = u32Width;
    stLayerAttr.stVidLayerDispWin.u16Height = u32Height;
    MI_DISP_SetVideoLayerAttr(DispLayer, &stLayerAttr);
    MI_DISP_EnableVideoLayer(DispLayer);

    memset(&stInputPortAttr, 0, sizeof(stInputPortAttr));
    stInputPortAttr.stDispWin.u16X      = 0;
    stInputPortAttr.stDispWin.u16Y      = 0;
    stInputPortAttr.stDispWin.u16Width  = u32Width;
    stInputPortAttr.stDispWin.u16Height = u32Height;
    MI_DISP_SetInputPortAttr(DispLayer, 0, &stInputPortAttr);
    MI_DISP_GetInputPortAttr(DispLayer, 0, &stInputPortAttr);
    MI_DISP_EnableInputPort(DispLayer, 0);
    MI_DISP_SetInputPortSyncMode (DispLayer, 0, E_MI_DISP_SYNC_MODE_FREE_RUN);
    
    //bind divp - disp
    memset(&stBindInfo, 0, sizeof(stSysBindInfo_t));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_DIVP;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = 0;
    stBindInfo.stSrcChnPort.u32PortId = 0;

    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DISP;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = 0;
    stBindInfo.stDstChnPort.u32PortId = 0;
    stBindInfo.u32SrcFrmrate = 0;
    stBindInfo.u32DstFrmrate = 0;
    Sys_Bind(&stBindInfo);

    Hdmi_Start(eHdmi,eHdmiTiming);
    
    //creat send frame thread
    gstVdecThread.bRunFlag = TRUE;
    gstVdecThread.vdecChn = 0;
    gstVdecThread.szFileName = filePath;
    pthread_create(&gstVdecThread.pt, NULL, VdecSendStream, (void *)&gstVdecThread);

    //wait sub cmd
    while(1)
    {
        for(n = 0; n < astTestInfo[u8TestNum].u8TestSubInfoNum; n++)
        {
            printf("[%d]%s\n",n,astTestInfo[u8TestNum].astTestSubInfo[n].SubInfoDesc);
        }
        u8TestSubNum = Get_Test_Num();

        if(0 == strncmp(astTestInfo[u8TestNum].astTestSubInfo[u8TestSubNum].SubInfoDesc,"pause",5))
        {
            g_PushDataStop = TRUE;    
        }
        else if(0 == strncmp(astTestInfo[u8TestNum].astTestSubInfo[u8TestSubNum].SubInfoDesc,"resume",6))
        {
            g_PushDataStop = FALSE;
        }
        else if(0 == strncmp(astTestInfo[u8TestNum].astTestSubInfo[u8TestSubNum].SubInfoDesc,"zoom",4))
        {
            MI_DIVP_GetChnAttr(0, &stDivpChnAttr);
            stDivpChnAttr.stCropRect.u16X = 670;
            stDivpChnAttr.stCropRect.u16Y = 28;
            stDivpChnAttr.stCropRect.u16Width = 1120;
            stDivpChnAttr.stCropRect.u16Height = 1610;
            MI_DIVP_SetChnAttr(0, &stDivpChnAttr);
        }
        else if(0 == strncmp(astTestInfo[u8TestNum].astTestSubInfo[u8TestSubNum].SubInfoDesc,"exit",4))
        {
            g_PushDataExit = TRUE;
            pthread_join(gstVdecThread.pt, NULL);
            gstVdecThread.pt = 0;

            memset(&stBindInfo, 0, sizeof(stSysBindInfo_t));
            stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VDEC;
            stBindInfo.stSrcChnPort.u32DevId = 0;
            stBindInfo.stSrcChnPort.u32ChnId = 0;
            stBindInfo.stSrcChnPort.u32PortId = 0;

            stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DIVP;
            stBindInfo.stDstChnPort.u32DevId = 0;
            stBindInfo.stDstChnPort.u32ChnId = 0;
            stBindInfo.stDstChnPort.u32PortId = 0;
            Sys_UnBind(&stBindInfo);
            
            memset(&stBindInfo, 0, sizeof(stSysBindInfo_t));
            stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_DIVP;
            stBindInfo.stSrcChnPort.u32DevId = 0;
            stBindInfo.stSrcChnPort.u32ChnId = 0;
            stBindInfo.stSrcChnPort.u32PortId = 0;

            stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DISP;
            stBindInfo.stDstChnPort.u32DevId = 0;
            stBindInfo.stDstChnPort.u32ChnId = 0;
            stBindInfo.stDstChnPort.u32PortId = 0;
            Sys_UnBind(&stBindInfo);

            MI_VDEC_StopChn(0);
            MI_VDEC_DestroyChn(0);

            MI_DIVP_StopChn(0);
            MI_DIVP_DestroyChn(0);

            MI_DISP_DisableInputPort(DispLayer, 0);
            MI_DISP_DisableVideoLayer(DispLayer);
            MI_DISP_UnBindVideoLayer(DispLayer, DispDev);
            MI_DISP_Disable(DispDev);

            MI_HDMI_Stop(0);
            MI_HDMI_Close(0);
            MI_HDMI_DeInit();
            break;
        }
    }

}

int main(int argc, char **argv)
{
    MI_U8 u8TestCase;
    MI_U8 n = 0;

    for(n = 0; n < (sizeof(astDispTestFunc)/sizeof(stDispTestFunc_t)); n++)
    {
        printf("[%d] %s\n",n,astDispTestFunc[n].desc);
    }   
    u8TestCase = Get_Test_Num();
 
    printf("start test func_%d\n",u8TestCase);
    astDispTestFunc[u8TestCase].test_func();
    
    printf("--------EXIT--------\n");
    return 0;
}


