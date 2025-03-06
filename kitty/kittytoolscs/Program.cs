using XDevkit;
using Microsoft.Test.Xbox.XDRPC;
using System.Text;


// yes i have literally 0 prior c# experience

IXboxManager xboxManager = new XboxManagerClass();
IXboxConsole? xbox = null;
try
{
    // first try commandline
    string console = args[0];
    xbox = xboxManager.OpenConsole(console);
    Console.WriteLine("connected to {0}", xbox.Name);
}
catch
{
    try
    {
        // then try default console
        xbox = xboxManager.OpenConsole(xboxManager.DefaultConsole);
        Console.WriteLine("connected to {0}", xbox.Name);
    } catch {
        Console.WriteLine("couldn't connect press any key to exit");
        Console.ReadKey();
        System.Environment.Exit(0);
    }
}
if (xbox == null)
{
    Console.WriteLine("xbox == null, press any key to exit");
    Console.ReadKey();
    System.Environment.Exit(0);
}

//4E 80 00 20
byte[] GetMemory(in IXboxConsole console, uint Address, uint Length)
{
    uint BytesRead = 0u;
    byte[] array = new byte[Length];
    console.DebugTarget.GetMemory(Address, Length, array, out BytesRead);
    console.DebugTarget.InvalidateMemoryCache(ExecutablePages: true, Address, Length);
    return array;
}

/*
void WriteMem(in IXboxConsole console, uint address, byte[] array)
{
    uint bw = 0;
    xbox.DebugTarget.SetMemory(address, (uint)array.Length, array, out bw);
    xbox.DebugTarget.InvalidateMemoryCache(true, address, (uint)array.Length);
}

WriteMem(xbox, 0x82791AD8, [0x4e, 0x80, 0x0, 0x20]); //ret out SpewOutputFunc
WriteMem(xbox, 0x827B04D4, [0x82, 0x79, 0x1a, 0x20]) //Set spew func to DefaultSpewFunc
;
*/

string ReadCString(in IXboxConsole xbox, in uint address)
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

void ExecuteCommand(string command)
{
    // tu 0 0x860992A0
    // tu 5 0x8609A848
    XDRPCExecutionOptions options = new XDRPCExecutionOptions(XDRPCMode.Title, 0x8609A848);
    XDRPCArrayArgumentInfo<byte[]> cmd = new XDRPCArrayArgumentInfo<byte[]>(Encoding.ASCII.GetBytes(command), ArgumentType.ByRef);
    uint num = xbox.ExecuteRPC<uint>(options, [cmd]);
}
string ClassName(uint address)
{
    uint stringAddress = ReadU32(xbox, address);
    //Console.WriteLine(stringAddress.ToString("X"));
    return ReadCString(xbox, stringAddress);
}

string TableName(uint tableAddress)
{
    uint stringAddress = ReadU32(xbox, tableAddress + 8);
    return ReadCString(xbox, stringAddress);
}
uint sizeOfSendProp = 0x4c;

uint PropAddress(uint tableAddress, int index)
{
    uint firstPropAddress = ReadU32(xbox, tableAddress);

    return (uint)(firstPropAddress + (index * sizeOfSendProp));
}

void PrintTable(in StreamWriter writer, in uint tableAddress, int indent)
{
    string indentString = new string('\t', indent);
    int numProps = (int)ReadU32(xbox, tableAddress + 4);
    for (int i = 0; i < numProps; i++)
    {
        uint addrProp = PropAddress(tableAddress, i);

        PropWrapper.PropWrapper prop = new PropWrapper.PropWrapper(xbox, addrProp);
        uint datatableAddress = prop.DataTableAddress();
        /*if (datatableAddress != 0x0)
        {
            writer.WriteLine(String.Format("{0}table: {1} offset: {2} type: {3}", indentString, prop.Name(), prop.Offset(), TableName(datatableAddress)));
            PrintTable(writer, datatableAddress, indent + 1);
        }
        else
        {
            PropWrapper.PropWrapper.EType type = prop.Type();
            int flags = prop.Flags();
            writer.WriteLine(String.Format("{0}member: {1} offset: {2} type: {3} bits: {4} info: {5}", indentString, prop.Name(), prop.Offset(), type.ToString(), prop.Bits(), PropWrapper.PropWrapper.FlagsToString(flags, type)));
        }*/

        if (datatableAddress != 0x0)
        {
            writer.WriteLine(String.Format("{0}table: {1} type: {2}", indentString, prop.Name(), TableName(datatableAddress)));
            PrintTable(writer, datatableAddress, indent + 1);
        }
        else
        {
            PropWrapper.PropWrapper.EType type = prop.Type();
            int flags = prop.Flags();
            writer.WriteLine(String.Format("{0}member: {1}", indentString, prop.Name()));
        }
    }
}


void DumpTables()
{
    StreamWriter outputFile = new StreamWriter("sendtables.txt");

    // title update 5: 0x8857C2B8
    // title update 0: 0x8857BE10
    XDRPCExecutionOptions options = new XDRPCExecutionOptions(XDRPCMode.Title, 0x8857C2B8);
    uint currentAddress = xbox.ExecuteRPC<uint>(options);
    Console.WriteLine(currentAddress);

    while (currentAddress != 0x0)
    {
        string currentClass = ClassName(currentAddress);
        Console.WriteLine("Dumping {0}", currentClass);
        uint tableAddress = ReadU32(xbox, currentAddress + 4);
        outputFile.WriteLine(String.Format("class {0} (type {1})", currentClass, TableName(tableAddress)));
        PrintTable(outputFile, tableAddress, 0);
        currentAddress = ReadU32(xbox, currentAddress + 8);
        //Console.WriteLine(currentAddress);
    }
    outputFile.Close();
}

xbox.DebugTarget.Console.OnStdNotify += (XboxDebugEventType EventCode, IXboxEventInfo EventInfo) =>
{
    switch (EventCode)
    {
        case XboxDebugEventType.DebugString:
            string msg = EventInfo.Info.Message;
            if (msg.Length > 0)
            {
                if (msg[msg.Length - 1] == '\n')
                {
                    Console.Write("[dbg] {0}",msg);
                }
                else
                {
                    Console.WriteLine("[dbg] {0}", msg);

                }
            }
            break;
    }
};

while (true)
{
    string? line = Console.ReadLine();
    if (line != null)
    {
        if (line.Length == 0) { continue; }
        if (line == "~~dumpsendtables")
        {
            DumpTables();
            continue;
        }
        if (line[line.Length - 1] != '\n')
        {
            line += '\n';
        }
        ExecuteCommand(line);
    }
}