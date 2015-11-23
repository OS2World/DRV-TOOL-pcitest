#define VERSION "1.0"
#define OEMHLP "OEMHLP$"

#define OEM_CAT  0x80
#define PCI_FUNC 0x0b
#define PCI_GET_BIOS_INFO   0
#define PCI_FIND_DEVICE     1
#define PCI_FIND_CLASS_CODE 2
#define PCI_READ_CONFIG     3
#define PCI_WRITE_CONFIG    4

#define TRUE  1
#define FALSE 0

typedef unsigned char  UCHAR;
typedef unsigned short USHORT;
typedef unsigned long  ULONG;


typedef struct _PCI_PARM {
   UCHAR PCISubFunc;
   union {
      struct {
         USHORT DeviceID;
         USHORT VendorID;
         UCHAR  Index;
      }Parm_Find_Dev;
      struct {
         ULONG  ClassCode;
         UCHAR  Index;
      }Parm_Find_ClassCode;
      struct {
         UCHAR  BusNum;
         UCHAR  DevFunc;
         UCHAR  ConfigReg;
         UCHAR  Size;
      }Parm_Read_Config;
      struct {
         UCHAR  BusNum;
         UCHAR  DevFunc;
         UCHAR  ConfigReg;
         UCHAR  Size;
         ULONG  Data;
      }Parm_Write_Config;
   };
} PCI_PARM;

typedef struct _PCI_DATA {
   UCHAR bReturn;
   union {
      struct {
         UCHAR HWMech;
         UCHAR MajorVer;
         UCHAR MinorVer;
         UCHAR LastBus;
      } Data_Bios_Info;
      struct {
         UCHAR  BusNum;
         UCHAR  DevFunc;
      }Data_Find_Dev;
      struct {
         ULONG  Data;
      }Data_Read_Config;
   };
} PCI_DATA;

/* CmdLineFlags */
#define CMDLINE_FIND_DEVICES  0x01
#define MAX_DEV                 32
#define MAX_FUNC                 8

enum sizes { BSIZE = 1, WSIZE=2, DWSIZE=4};

UCHAR PCILastBus;

int ParseCmdLine(int argc, char *argv[]);
void Do_CmdLine(int CmdLineFlags);
void Do_Menu(void);
void FindAllPCIDevices(void);
void ReadConfigSpace(void);
UCHAR IsValidDevice(UCHAR, UCHAR);
UCHAR IsSingleFunc(UCHAR, UCHAR);
void PrintDevInfo(UCHAR, UCHAR, UCHAR);
void FindClassCode(void);
void FindDevice(void);
void DumpDeviceConfigSpace(void);
