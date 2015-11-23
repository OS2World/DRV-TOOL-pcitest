#include <stdio.h>
#include <math.h>
#include <doscalls.h>
#include <sysbits.h>                  /* bit definitions                    */
#include "pcitest.h"


unsigned   out_handle,action; /* for DosOpen  */
PCI_PARM   Pci_parm;
PCI_DATA   Pci_data;

void main(argc, argv, envp)
   int argc;
   char *argv[];
   char *envp[];
{
  int        rc=0;
  int        CmdLineFlags=0;

  printf("\nPCITEST Version %s.\n",VERSION);

  rc = DOSOPEN((char far *)OEMHLP,     /* device name to open               */
     (unsigned far *)&out_handle,      /* new handle                        */
     (unsigned far *)&action,          /* action taken                      */
     (unsigned long)0,                 /* primary allocation                */
     0,                                /* + read only                       */
     OF_EXISTING_FILE,                 /* don't create a file               */
     OM_DENY_NONE+OM_WRITE_ONLY,       /* !!!!! ugh !!!!!!!!                */
     (unsigned long)0);                /* reserved                          */

  if(rc){   /* if fails */
    printf("DosOpen for %s: rc = %u.\n",OEMHLP,rc);
    return;
  }

  /* PCI BIOS Presence Check */

  /* Set up Parm */
  Pci_parm.PCISubFunc = PCI_GET_BIOS_INFO;
  rc = DOSDEVIOCTL2((char far *)&Pci_data,
       (unsigned)sizeof(Pci_data),
       (char far *)&Pci_parm,
       (unsigned)sizeof(Pci_parm),
       (unsigned short)PCI_FUNC,
       (unsigned short)OEM_CAT,
       (unsigned short)out_handle);


  if (!rc) {
     printf("PCI BIOS Installed.\n");
     printf("\tPCI BIOS Version:%u.%u\n",
             Pci_data.Data_Bios_Info.MajorVer,
             Pci_data.Data_Bios_Info.MinorVer );
     printf("\tHardware Mechanism:%u\n",Pci_data.Data_Bios_Info.HWMech);
     PCILastBus = Pci_data.Data_Bios_Info.LastBus;
     printf("\tLast Bus:%u\n",Pci_data.Data_Bios_Info.LastBus);
  } else {
     printf("PCIBIOSInfo call: rc = %u.\n",rc);
     return;
  }
  if(argc>1)
     CmdLineFlags = ParseCmdLine(argc-1, &argv[1]);

  if (CmdLineFlags)
     Do_CmdLine(CmdLineFlags);
  else
     Do_Menu();

}

int ParseCmdLine(int argc, char *argv[])
{
   int i,CmdLineFlags=0;

   for (i=0; i<argc; i++) {
      if (argv[i][0] == '/')
        if ((argv[i][1] == 'f') || (argv[i][1] == 'F'))
           CmdLineFlags |= CMDLINE_FIND_DEVICES;
   }
   return(CmdLineFlags);
}

void Do_CmdLine(int CmdLineFlags)
{
   if (CmdLineFlags & CMDLINE_FIND_DEVICES)
      FindAllPCIDevices();

}

void Do_Menu()
{
   UCHAR Response='1';

   while (Response != '0') {
      if ((Response >= '0') && (Response <='5')) {
         printf("\n0) Quit\n");
         printf("1) Find All PCI Devices.\n");
         printf("2) Read Configuration Space.\n");
         printf("3) Find Device by Class Code.\n");
         printf("4) Find Device by DeviceID and VendorID.\n");
         printf("5) Dump Device's Config Space.\n");

      }
      Response = (UCHAR)getchar();
      switch(Response){
      case '1':
         FindAllPCIDevices();
         break;
      case '2':
         ReadConfigSpace();
         break;
      case '3':
         FindClassCode();
         break;
      case '4':
         FindDevice();
         break;
      case '5':
         DumpDeviceConfigSpace();
         break;
      default:
         break;
      }
   }

}

void FindAllPCIDevices()
{
   UCHAR BusNum,Dev,FuncLimit,Func;


   for (BusNum=0; BusNum<=PCILastBus; BusNum++) {
      for (Dev=0; Dev<MAX_DEV; Dev++) {
         if (!(IsValidDevice(BusNum, Dev)))
            continue;

         /* We have a valid device, now need to know if single or */
         /* or multi-function */

         if (IsSingleFunc(BusNum, Dev))
            FuncLimit=1;
         else
            FuncLimit=8;

         for (Func=0; Func<FuncLimit; Func++){
            PrintDevInfo(BusNum, Dev, Func);
         }

      }
   }

}


void DumpDeviceConfigSpace()
{
  int rc,i;
  UCHAR BusNum,DevFunc;

  while(1){
     BusNum=0xff; DevFunc=0xff;
     printf("\nEnter BusNum,DevFunc: ");
     rc = scanf("%x,%x",&BusNum,&DevFunc);
     if (rc != 2){
        printf("rc = %d. Bad Input: %x,%x.  Try again.\n",rc,BusNum,DevFunc);
        continue;
     }
     if (BusNum >  PCILastBus) {
        printf("\nBad BusNum. LastBus=%x",PCILastBus);
        continue;
     }
     if (DevFunc>0xff){
        printf("\nBad DevFunc. DevFunc = %x. Must be less than 0100 Hex.",DevFunc);
        continue;
     }
     break;
  }

  /* Set up Parm */
  Pci_parm.PCISubFunc = PCI_READ_CONFIG;
  Pci_parm.Parm_Read_Config.BusNum    = BusNum;
  Pci_parm.Parm_Read_Config.DevFunc   = DevFunc;
  Pci_parm.Parm_Read_Config.Size      = BSIZE;

  for (i=0;(i+3)<=255 ;i+=4 ) {
     Pci_parm.Parm_Read_Config.ConfigReg = (UCHAR)(i+3);
     rc = DOSDEVIOCTL2((char far *)&Pci_data,
          (unsigned)sizeof(Pci_data),
          (char far *)&Pci_parm,
          (unsigned)sizeof(Pci_parm),
          (unsigned short)PCI_FUNC,
          (unsigned short)OEM_CAT,
          (unsigned short)out_handle);


  if (rc){
     printf("ReadConfig failed \n");
  } else
     printf("\t%x",Pci_data.Data_Read_Config.Data);

     Pci_parm.Parm_Read_Config.ConfigReg = (UCHAR)(i+2);
     rc = DOSDEVIOCTL2((char far *)&Pci_data,
          (unsigned)sizeof(Pci_data),
          (char far *)&Pci_parm,
          (unsigned)sizeof(Pci_parm),
          (unsigned short)PCI_FUNC,
          (unsigned short)OEM_CAT,
          (unsigned short)out_handle);


  if (rc){
     printf("ReadConfig failed \n");
  } else
     printf("\t%x",Pci_data.Data_Read_Config.Data);

     Pci_parm.Parm_Read_Config.ConfigReg = (UCHAR)(i+1);
     rc = DOSDEVIOCTL2((char far *)&Pci_data,
          (unsigned)sizeof(Pci_data),
          (char far *)&Pci_parm,
          (unsigned)sizeof(Pci_parm),
          (unsigned short)PCI_FUNC,
          (unsigned short)OEM_CAT,
          (unsigned short)out_handle);


  if (rc){
     printf("ReadConfig failed \n");
  } else
     printf("\t%x",Pci_data.Data_Read_Config.Data);

     Pci_parm.Parm_Read_Config.ConfigReg = (UCHAR)i;
     rc = DOSDEVIOCTL2((char far *)&Pci_data,
          (unsigned)sizeof(Pci_data),
          (char far *)&Pci_parm,
          (unsigned)sizeof(Pci_parm),
          (unsigned short)PCI_FUNC,
          (unsigned short)OEM_CAT,
          (unsigned short)out_handle);


  if (rc){
     printf("ReadConfig failed \n");
  } else
     printf("\t%x",Pci_data.Data_Read_Config.Data);

  printf("\t%x\n",i);

  } /* endfor */



}

void ReadConfigSpace()
{
  int rc;
  UCHAR BusNum,DevFunc,ConfigReg,Size;



  while(1){
     BusNum=0xff; DevFunc=0xff; ConfigReg=0; Size=3;
     printf("\nEnter BusNum,DevFunc,ConfigReg,Size");
     printf("\nAll numbers in hex.  Size = 1,2,or 4.\n");
     rc = scanf("%x,%x,%x,%d",&BusNum,&DevFunc,&ConfigReg,&Size);
     if (rc != 4){
        printf("rc = %d. Bad Input: %x,%x,%x,%d.  Try again.\n",rc,BusNum,DevFunc,ConfigReg,Size);
        continue;
     }
     if (BusNum >  PCILastBus) {
        printf("\nBad BusNum. LastBus=%x",PCILastBus);
        continue;
     }
     if (DevFunc>0xff){
        printf("\nBad DevFunc. DevFunc = %x. Must be less than 0100 Hex.",DevFunc);
        continue;
     }
     if (ConfigReg>0xff){
        printf("\nBad ConfigReg. ConfigReg = %x. Must be less than 0100 Hex.",ConfigReg);
        continue;
     }
     if (!((Size==BSIZE) || (Size == WSIZE) || (Size == DWSIZE))){
        printf("\nSize = %d.  Must be 1, 2, or 4.",Size);
        continue;
     }
     break;
  }


  /* Set up Parm */
  Pci_parm.PCISubFunc = PCI_READ_CONFIG;
  Pci_parm.Parm_Read_Config.BusNum    = BusNum;
  Pci_parm.Parm_Read_Config.DevFunc   = DevFunc;
  Pci_parm.Parm_Read_Config.ConfigReg = ConfigReg;
  Pci_parm.Parm_Read_Config.Size      = Size;

  rc = DOSDEVIOCTL2((char far *)&Pci_data,
       (unsigned)sizeof(Pci_data),
       (char far *)&Pci_parm,
       (unsigned)sizeof(Pci_parm),
       (unsigned short)PCI_FUNC,
       (unsigned short)OEM_CAT,
       (unsigned short)out_handle);

   if (rc){
      printf("ReadConfig failed \n");
   } else if (Size != DWSIZE){
      printf("Word read = %x\n",Pci_data.Data_Read_Config.Data);
   } else {
      printf("Word read = %x%x\n",
      *((USHORT *)&Pci_data.Data_Read_Config.Data+1),
      Pci_data.Data_Read_Config.Data);
   }


}

UCHAR IsValidDevice(UCHAR BusNum, UCHAR Dev)
{
   int rc;


   /* Set up Parm */
   Pci_parm.PCISubFunc = PCI_READ_CONFIG;
   Pci_parm.Parm_Read_Config.BusNum    = BusNum;
   Pci_parm.Parm_Read_Config.DevFunc   = (UCHAR)(Dev << 3);
   Pci_parm.Parm_Read_Config.ConfigReg = 0x00;
   Pci_parm.Parm_Read_Config.Size      = WSIZE;

   rc = DOSDEVIOCTL2((char far *)&Pci_data,
        (unsigned)sizeof(Pci_data),
        (char far *)&Pci_parm,
        (unsigned)sizeof(Pci_parm),
        (unsigned short)PCI_FUNC,
        (unsigned short)OEM_CAT,
        (unsigned short)out_handle);
   if ((rc) ||
       (Pci_data.bReturn != 0) ||
       (Pci_data.Data_Read_Config.Data) == 0xffff)
      return(FALSE);

   return(TRUE);

}

UCHAR IsSingleFunc(UCHAR BusNum, UCHAR Dev)
{
   int rc;


   /* Set up Parm */
   Pci_parm.PCISubFunc = PCI_READ_CONFIG;
   Pci_parm.Parm_Read_Config.BusNum    = BusNum;
   Pci_parm.Parm_Read_Config.DevFunc   = (UCHAR)(Dev << 3);
   Pci_parm.Parm_Read_Config.ConfigReg = 0x0e;  /* header type */
   Pci_parm.Parm_Read_Config.Size      = BSIZE;

   rc = DOSDEVIOCTL2((char far *)&Pci_data,
        (unsigned)sizeof(Pci_data),
        (char far *)&Pci_parm,
        (unsigned)sizeof(Pci_parm),
        (unsigned short)PCI_FUNC,
        (unsigned short)OEM_CAT,
        (unsigned short)out_handle);
   if ((rc) ||
       (Pci_data.bReturn != 0) ||
       (Pci_data.Data_Read_Config.Data & 0x80))  /* multi-func bit */
      return(FALSE);

   return(TRUE);

}

void PrintDevInfo(UCHAR BusNum, UCHAR Dev, UCHAR Func)
{
   int rc;
   UCHAR DevFunc = (UCHAR)((Dev<<3)+Func);


   /* Set up Parm */
   Pci_parm.PCISubFunc = PCI_READ_CONFIG;
   Pci_parm.Parm_Read_Config.BusNum    = BusNum;
   Pci_parm.Parm_Read_Config.DevFunc   = DevFunc;
   Pci_parm.Parm_Read_Config.ConfigReg = 0x00;  /* Vendor ID */
   Pci_parm.Parm_Read_Config.Size      = WSIZE;

   rc = DOSDEVIOCTL2((char far *)&Pci_data,
        (unsigned)sizeof(Pci_data),
        (char far *)&Pci_parm,
        (unsigned)sizeof(Pci_parm),
        (unsigned short)PCI_FUNC,
        (unsigned short)OEM_CAT,
        (unsigned short)out_handle);
   if ((rc) ||
       (Pci_data.bReturn != 0))
      return;

   printf("\n\n\nBus Number: %x\tDevFunc: %x",BusNum,DevFunc);
   printf("\nVendor ID: %x",Pci_data.Data_Read_Config.Data);

   /* Set up Parm */
   Pci_parm.PCISubFunc = PCI_READ_CONFIG;
   Pci_parm.Parm_Read_Config.BusNum    = BusNum;
   Pci_parm.Parm_Read_Config.DevFunc   = DevFunc;
   Pci_parm.Parm_Read_Config.ConfigReg = 0x02;  /* Device ID */
   Pci_parm.Parm_Read_Config.Size      = WSIZE;

   rc = DOSDEVIOCTL2((char far *)&Pci_data,
        (unsigned)sizeof(Pci_data),
        (char far *)&Pci_parm,
        (unsigned)sizeof(Pci_parm),
        (unsigned short)PCI_FUNC,
        (unsigned short)OEM_CAT,
        (unsigned short)out_handle);
   if ((rc) ||
       (Pci_data.bReturn != 0))
      return;

   printf("\tDevice ID: %x",Pci_data.Data_Read_Config.Data);

   /* Set up Parm */
   Pci_parm.PCISubFunc = PCI_READ_CONFIG;
   Pci_parm.Parm_Read_Config.BusNum    = BusNum;
   Pci_parm.Parm_Read_Config.DevFunc   = DevFunc;
   Pci_parm.Parm_Read_Config.ConfigReg = 0x0b;  /* Device ID */
   Pci_parm.Parm_Read_Config.Size      = BSIZE;

   rc = DOSDEVIOCTL2((char far *)&Pci_data,
        (unsigned)sizeof(Pci_data),
        (char far *)&Pci_parm,
        (unsigned)sizeof(Pci_parm),
        (unsigned short)PCI_FUNC,
        (unsigned short)OEM_CAT,
        (unsigned short)out_handle);
   if ((rc) ||
       (Pci_data.bReturn != 0))
      return;

   printf("\nClass Code: %x",Pci_data.Data_Read_Config.Data);

   /* Set up Parm */
   Pci_parm.PCISubFunc = PCI_READ_CONFIG;
   Pci_parm.Parm_Read_Config.BusNum    = BusNum;
   Pci_parm.Parm_Read_Config.DevFunc   = DevFunc;
   Pci_parm.Parm_Read_Config.ConfigReg = 0x0a;  /* Device ID */
   Pci_parm.Parm_Read_Config.Size      = BSIZE;

   rc = DOSDEVIOCTL2((char far *)&Pci_data,
        (unsigned)sizeof(Pci_data),
        (char far *)&Pci_parm,
        (unsigned)sizeof(Pci_parm),
        (unsigned short)PCI_FUNC,
        (unsigned short)OEM_CAT,
        (unsigned short)out_handle);
   if ((rc) ||
       (Pci_data.bReturn != 0))
      return;

   printf(" %x",Pci_data.Data_Read_Config.Data);

   /* Set up Parm */
   Pci_parm.PCISubFunc = PCI_READ_CONFIG;
   Pci_parm.Parm_Read_Config.BusNum    = BusNum;
   Pci_parm.Parm_Read_Config.DevFunc   = DevFunc;
   Pci_parm.Parm_Read_Config.ConfigReg = 0x09;  /* Device ID */
   Pci_parm.Parm_Read_Config.Size      = BSIZE;

   rc = DOSDEVIOCTL2((char far *)&Pci_data,
        (unsigned)sizeof(Pci_data),
        (char far *)&Pci_parm,
        (unsigned)sizeof(Pci_parm),
        (unsigned short)PCI_FUNC,
        (unsigned short)OEM_CAT,
        (unsigned short)out_handle);
   if ((rc) ||
       (Pci_data.bReturn != 0))
      return;

   printf(" %x",Pci_data.Data_Read_Config.Data);

   /* Set up Parm */
   Pci_parm.PCISubFunc = PCI_READ_CONFIG;
   Pci_parm.Parm_Read_Config.BusNum    = BusNum;
   Pci_parm.Parm_Read_Config.DevFunc   = DevFunc;
   Pci_parm.Parm_Read_Config.ConfigReg = 0x3d;  /* Device ID */
   Pci_parm.Parm_Read_Config.Size      = BSIZE;

   rc = DOSDEVIOCTL2((char far *)&Pci_data,
        (unsigned)sizeof(Pci_data),
        (char far *)&Pci_parm,
        (unsigned)sizeof(Pci_parm),
        (unsigned short)PCI_FUNC,
        (unsigned short)OEM_CAT,
        (unsigned short)out_handle);
   if ((rc) ||
       (Pci_data.bReturn != 0))
      return;

   printf("\nInterrupt Pin: %x",Pci_data.Data_Read_Config.Data);

   /* Set up Parm */
   if(Pci_data.Data_Read_Config.Data  != 0){
      Pci_parm.PCISubFunc = PCI_READ_CONFIG;
      Pci_parm.Parm_Read_Config.BusNum    = BusNum;
      Pci_parm.Parm_Read_Config.DevFunc   = DevFunc;
      Pci_parm.Parm_Read_Config.ConfigReg = 0x3c;  /* Device ID */
      Pci_parm.Parm_Read_Config.Size      = BSIZE;

      rc = DOSDEVIOCTL2((char far *)&Pci_data,
           (unsigned)sizeof(Pci_data),
           (char far *)&Pci_parm,
           (unsigned)sizeof(Pci_parm),
           (unsigned short)PCI_FUNC,
           (unsigned short)OEM_CAT,
           (unsigned short)out_handle);
      if ((rc) ||
          (Pci_data.bReturn != 0))
         return;

      printf("\tInterrupt Line: %x",Pci_data.Data_Read_Config.Data);
   }

   printf("\n");
   return;
}

void FindClassCode()
{
  int rc;
  ULONG ClassCode;
  UCHAR Func,SubFunc,IntF,Index;

  while(1){
     printf("\nEnter Class Code: Func,SubFunc,I/f,Index");
     printf("\nAll numbers in hex.\n");
     rc = scanf("%x,%x,%x,%x",&Func,&SubFunc,&IntF,&Index);
     if (rc != 4){
        printf("rc = %d. Bad Input: %x,%x,%x,%d.  Try again.\n",rc,Func,SubFunc,IntF,Index);
        continue;
     }
     break;
  }

  ClassCode =  ((ULONG)Func << 16);
  ClassCode += ((ULONG)SubFunc << 8);
  ClassCode += (ULONG)IntF;

  /* Set up Parm */
  Pci_parm.PCISubFunc = PCI_FIND_CLASS_CODE;
  Pci_parm.Parm_Find_ClassCode.ClassCode = ClassCode;
  Pci_parm.Parm_Find_ClassCode.Index = Index;

  rc = DOSDEVIOCTL2((char far *)&Pci_data,
       (unsigned)sizeof(Pci_data),
       (char far *)&Pci_parm,
       (unsigned)sizeof(Pci_parm),
       (unsigned short)PCI_FUNC,
       (unsigned short)OEM_CAT,
       (unsigned short)out_handle);

   if (rc){
      printf("Find Device failed: ReadConfig failed \n");
   } else {
      printf("\nBusNum = %x",Pci_data.Data_Find_Dev.BusNum);
      printf("\tDevFunc = %x\n",Pci_data.Data_Find_Dev.DevFunc);
   }

}

void FindDevice()
{
  int rc;
  USHORT DeviceID, VendorID;
  UCHAR Index;

  while(1){
     printf("\nEnter DevID, VendID, Index");
     printf("\nAll numbers in hex.\n");
     rc = scanf("%x,%x,%x",&DeviceID,&VendorID,&Index);
     if (rc != 3){
        printf("rc = %d. Bad Input: %x,%x,%x.  Try again.\n",rc,DeviceID,VendorID,Index);
        continue;
     }
     break;
  }


  /* Set up Parm */
  Pci_parm.PCISubFunc = PCI_FIND_DEVICE;
  Pci_parm.Parm_Find_Dev.DeviceID = DeviceID;
  Pci_parm.Parm_Find_Dev.VendorID = VendorID;
  Pci_parm.Parm_Find_Dev.Index = Index;

  rc = DOSDEVIOCTL2((char far *)&Pci_data,
       (unsigned)sizeof(Pci_data),
       (char far *)&Pci_parm,
       (unsigned)sizeof(Pci_parm),
       (unsigned short)PCI_FUNC,
       (unsigned short)OEM_CAT,
       (unsigned short)out_handle);

   if (rc){
      printf("Find Device failed: ReadConfig failed \n");
   } else {
      printf("\nBusNum = %x",Pci_data.Data_Find_Dev.BusNum);
      printf("\tDevFunc = %x\n",Pci_data.Data_Find_Dev.DevFunc);
   }

}
