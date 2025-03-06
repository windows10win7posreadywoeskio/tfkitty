using XDevkit;
using System.Text;
namespace PropWrapper
{
    public class PropWrapper
    {

        static public string FlagsToString(int flags, EType type)
        {
            string ret = new string("");

            if ((flags & (1 << 0)) != 0)
            {
                ret += "Unsigned|";
            }
            if ((flags & (1 << 1)) != 0)
            {
                ret += "Coord|";
            }
            if ((flags & (1 << 2)) != 0)
            {
                ret += "NoScale|";
            }
            if ((flags & (1 << 3)) != 0)
            {
                ret += "RoundDown|";
            }
            if ((flags & (1 << 4)) != 0)
            {
                ret += "RoundUp|";
            }
            if ((flags & (1 << 5)) != 0)
            {
                if (type == EType.DPT_Int)
                {
                    ret += "VarInt|";
                }
                else
                {
                    ret += "Normal|";
                }
            }
            if ((flags & (1 << 6)) != 0)
            {
                ret += "Exclude|";
            }
            if ((flags & (1 << 7)) != 0)
            {
                ret += "XYZE|";
            }
            if ((flags & (1 << 8)) != 0)
            {
                ret += "InsideArray|";
            }
            if ((flags & (1 << 9)) != 0)
            {
                ret += "AlwaysProxy|";
            }
            if ((flags & (1 << 10)) != 0)
            {
                ret += "ChangesOften|";
            }
            if ((flags & (1 << 11)) != 0)
            {
                ret += "VectorElem|";
            }
            if ((flags & (1 << 12)) != 0)
            {
                ret += "Collapsible|";
            }
            if ((flags & (1 << 13)) != 0)
            {
                ret += "CoordMP|";
            }
            if ((flags & (1 << 14)) != 0)
            {
                ret += "CoordMPLowPrec|";
            }
            if ((flags & (1 << 15)) != 0)
            {
                ret += "CoordMpIntegral|";
            }
            return ret;

        }

        public enum EType
        {
            DPT_Int = 0x0,
            DPT_Float = 0x1,
            DPT_Vector = 0x2,
            DPT_String = 0x3,
            DPT_Array = 0x4,
            DPT_DataTable = 0x5,
            DPT_NUMSendPropTypes = 0x6,
        }

        /*
            FFFFFFFF enum SendPropType : __int32
            FFFFFFFF {                                       // XREF: DVariant/r RecvProp/r ...
            FFFFFFFF     DPT_Int              = 0x0,
            FFFFFFFF     DPT_Float            = 0x1,
            FFFFFFFF     DPT_Vector           = 0x2,
            FFFFFFFF     DPT_String           = 0x3,
            FFFFFFFF     DPT_Array            = 0x4,
            FFFFFFFF     DPT_DataTable        = 0x5,
            FFFFFFFF     DPT_NUMSendPropTypes = 0x6,
            FFFFFFFF };
        */

        /*
            00000000 struct __cppobj SendProp // sizeof=0x4C
            00000000 {
            00000000     SendProp_vtbl *__vftable;
            00000004     RecvProp *m_pMatchingRecvProp;
            00000008     SendPropType m_Type;
            0000000C     int m_nBits;
            00000010     float m_fLowValue;
            00000014     float m_fHighValue;
            00000018     SendProp *m_pArrayProp;
            0000001C     int (__cdecl *m_ArrayLengthProxy)(const void *, int);
            00000020     int m_nElements;
            00000024     int m_ElementStride;
            00000028     $F9977FCDF225486CB3FAB0441127CFAC ___u10;
            0000002C     char *m_pVarName;
            00000030     float m_fHighLowMul;
            00000034     int m_Flags;
            00000038     void (__cdecl *m_ProxyFn)(const SendProp *, const void *, const void *, DVariant *, int, int);
            0000003C     void *(__cdecl *m_DataTableProxyFn)(const SendProp *, const void *, const void *, CSendProxyRecipients *, int);
            00000040     SendTable *m_pDataTable;
            00000044     int m_Offset;
            00000048     const void *m_pExtraData;
            0000004C };
        */

        IXboxConsole xboxConsole;
        public uint propAddress;
        public byte[] GetMemory(in IXboxConsole console, uint Address, uint Length)
        {
            uint BytesRead = 0u;
            byte[] array = new byte[Length];
            console.DebugTarget.GetMemory(Address, Length, array, out BytesRead);
            console.DebugTarget.InvalidateMemoryCache(ExecutablePages: true, Address, Length);
            return array;
        }
        public string ReadCString(in IXboxConsole xbox, in uint address)
        {
            List<byte> bytes = new List<byte>();
            uint currentAddress = address;

            byte currentByte = GetMemory(xbox, currentAddress, 1)[0];
            while (currentByte != 0x0)
            {
                bytes.Add(currentByte);
                currentAddress++;
                currentByte = GetMemory(xbox, currentAddress, 1)[0];
            }
            return Encoding.UTF8.GetString(bytes.ToArray());
        }
        uint ReadU32(in IXboxConsole xbox, in uint address)
        {
            byte[] mem = GetMemory(xbox, address, 4);
            return BitConverter.ToUInt32(mem.Reverse().ToArray());
        }


        public PropWrapper(IXboxConsole _console, uint _propAddress)
        {
            this.xboxConsole = _console;
            this.propAddress = _propAddress;
        }

        public string Name()
        {
            uint stringAddress = ReadU32(xboxConsole, propAddress + 0x2c);
            //Console.WriteLine("name address {0}", stringAddress.ToString("X"));
            return ReadCString(xboxConsole, stringAddress);
        }

        public int Offset()
        {
            return (int)ReadU32(xboxConsole, propAddress + 0x44);
        }

        public EType Type()
        {
            return (EType)ReadU32(xboxConsole, propAddress + 0x8);
        }

        public int Bits()
        {
            return (int)ReadU32(xboxConsole, propAddress + 0x0C);
        }

        public int Flags()
        {
            return (int)ReadU32(xboxConsole, propAddress + 0x34);
        }
        public uint DataTableAddress()
        {
            return ReadU32(xboxConsole, propAddress + 0x40);
        }
    }
}