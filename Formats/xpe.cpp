// copyright (c) 2017-2019 hors<horsicq@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
#include "xpe.h"

XPE::XPE(QIODevice *__pDevice, bool bIsImage, qint64 nImageBase): XMSDOS(__pDevice,bIsImage,nImageBase)
{
}

bool XPE::isValid()
{
    bool bResult=false;

    quint16 magic=get_magic();

    if(magic==(quint16)XMSDOS_DEF::S_IMAGE_DOS_SIGNATURE)
    {
        qint32 lfanew=get_lfanew();

        if(lfanew>0)
        {
            quint32 signature=read_uint32(lfanew);

            if(signature==XPE_DEF::S_IMAGE_NT_SIGNATURE)
            {
                bResult=true;
            }
        }
    }

    return bResult;
}

bool XPE::is64()
{
    quint16 nMachine=getFileHeader_Machine();

    return  (nMachine==XPE_DEF::S_IMAGE_FILE_MACHINE_AMD64)||
            (nMachine==XPE_DEF::S_IMAGE_FILE_MACHINE_IA64)||
            (nMachine==XPE_DEF::S_IMAGE_FILE_MACHINE_ARM64);
}

bool XPE::isRichSignaturePresent()
{
    bool bResult=false;
    int nOffset=sizeof(XMSDOS_DEF::IMAGE_DOS_HEADER);
    int nSize=get_lfanew()-sizeof(XMSDOS_DEF::IMAGE_DOS_HEADER);

    if((nSize>0)&&(nSize<=0x200))
    {
        QByteArray baStub=read_array(nOffset,nSize);

        bResult=baStub.contains("Rich");
    }

    return bResult;
}

QList<XPE::RICH_RECORD> XPE::getRichSignatureRecords()
{
    QList<RICH_RECORD> listResult;

    qint64 nOffset=find_ansiString(getDosStubOffset(),getDosStubSize(),"Rich");

    if(nOffset!=-1)
    {
        quint32 nXORkey=read_uint32(nOffset+4);

        qint64 nCurrentOffset=nOffset-4;

        while(nCurrentOffset>getDosStubOffset()) // TODO optimize
        {
            quint32 nTemp=read_uint32(nCurrentOffset)^nXORkey;

            if(nTemp==0x536e6144) // DanS
            {
                nCurrentOffset+=16;

                for(; nCurrentOffset<nOffset; nCurrentOffset+=8)
                {
                    RICH_RECORD record;

                    quint32 nValue1=read_uint32(nCurrentOffset)^nXORkey;
                    record.nId=nValue1>>16;
                    record.nVersion=nValue1&0xFFFF;

                    //                    quint32 n1=nValue1>>24;
                    //                    quint32 n2=(nValue1>>16)&0xFF;

                    quint32 nValue2=read_uint32(nCurrentOffset+4)^nXORkey;
                    record.nCount=nValue2;

                    listResult.append(record);
                }

                break;
            }

            nCurrentOffset-=4;
        }
    }

    return listResult;
}

qint64 XPE::getDosStubSize()
{
    qint64 nSize=(qint64)get_lfanew()-sizeof(XMSDOS_DEF::IMAGE_DOS_HEADEREX);

    nSize=qMax(nSize,(qint64)0);

    return nSize;
}

qint64 XPE::getDosStubOffset()
{
    return sizeof(XMSDOS_DEF::IMAGE_DOS_HEADEREX);
}

QByteArray XPE::getDosStub()
{
    return read_array(getDosStubOffset(),getDosStubSize());
}

bool XPE::isDosStubPresent()
{
    return getDosStubSize()!=0;
}

qint64 XPE::getNtHeadersOffset()
{
    qint64 result=get_lfanew();

    if(!_isOffsetValid(result))
    {
        result=-1;
    }

    return result;
}

quint32 XPE::getNtHeaders_Signature()
{
    qint64 nOffset=getNtHeadersOffset();

    return read_uint32(nOffset);
}

void XPE::setNtHeaders_Signature(quint32 value)
{
    write_uint32(getNtHeadersOffset(),value);
}

qint64 XPE::getFileHeaderOffset()
{
    qint64 result=get_lfanew()+4;

    if(!_isOffsetValid(result))
    {
        result=-1;
    }

    return result;
}

XPE_DEF::S_IMAGE_FILE_HEADER XPE::getFileHeader()
{
    XPE_DEF::S_IMAGE_FILE_HEADER result= {};

    read_array(getFileHeaderOffset(),(char *)&result,sizeof(XPE_DEF::S_IMAGE_FILE_HEADER));

    return result;
}

void XPE::setFileHeader(XPE_DEF::S_IMAGE_FILE_HEADER *pFileHeader)
{
    write_array(getFileHeaderOffset(),(char *)pFileHeader,sizeof(XPE_DEF::S_IMAGE_FILE_HEADER));
}

quint16 XPE::getFileHeader_Machine()
{
    return read_uint16(getFileHeaderOffset()+offsetof(XPE_DEF::S_IMAGE_FILE_HEADER,Machine));
}

quint16 XPE::getFileHeader_NumberOfSections()
{
    return read_uint16(getFileHeaderOffset()+offsetof(XPE_DEF::S_IMAGE_FILE_HEADER,NumberOfSections));
}

quint32 XPE::getFileHeader_TimeDateStamp()
{
    return read_uint32(getFileHeaderOffset()+offsetof(XPE_DEF::S_IMAGE_FILE_HEADER,TimeDateStamp));
}

quint32 XPE::getFileHeader_PointerToSymbolTable()
{
    return read_uint32(getFileHeaderOffset()+offsetof(XPE_DEF::S_IMAGE_FILE_HEADER,PointerToSymbolTable));
}

quint32 XPE::getFileHeader_NumberOfSymbols()
{
    return read_uint32(getFileHeaderOffset()+offsetof(XPE_DEF::S_IMAGE_FILE_HEADER,NumberOfSymbols));
}

quint16 XPE::getFileHeader_SizeOfOptionalHeader()
{
    return read_uint16(getFileHeaderOffset()+offsetof(XPE_DEF::S_IMAGE_FILE_HEADER,SizeOfOptionalHeader));
}

quint16 XPE::getFileHeader_Characteristics()
{
    return read_uint16(getFileHeaderOffset()+offsetof(XPE_DEF::S_IMAGE_FILE_HEADER,Characteristics));
}

void XPE::setFileHeader_Machine(quint16 value)
{
    write_uint16(getFileHeaderOffset()+offsetof(XPE_DEF::S_IMAGE_FILE_HEADER,Machine),value);
}

void XPE::setFileHeader_NumberOfSections(quint16 value)
{
    write_uint16(getFileHeaderOffset()+offsetof(XPE_DEF::S_IMAGE_FILE_HEADER,NumberOfSections),value);
}

void XPE::setFileHeader_TimeDateStamp(quint32 value)
{
    write_uint32(getFileHeaderOffset()+offsetof(XPE_DEF::S_IMAGE_FILE_HEADER,TimeDateStamp),value);
}

void XPE::setFileHeader_PointerToSymbolTable(quint32 value)
{
    write_uint32(getFileHeaderOffset()+offsetof(XPE_DEF::S_IMAGE_FILE_HEADER,PointerToSymbolTable),value);
}

void XPE::setFileHeader_NumberOfSymbols(quint32 value)
{
    write_uint32(getFileHeaderOffset()+offsetof(XPE_DEF::S_IMAGE_FILE_HEADER,NumberOfSymbols),value);
}

void XPE::setFileHeader_SizeOfOptionalHeader(quint16 value)
{
    write_uint16(getFileHeaderOffset()+offsetof(XPE_DEF::S_IMAGE_FILE_HEADER,SizeOfOptionalHeader),value);
}

void XPE::setFileHeader_Characteristics(quint16 value)
{
    write_uint16(getFileHeaderOffset()+offsetof(XPE_DEF::S_IMAGE_FILE_HEADER,Characteristics),value);
}

qint64 XPE::getOptionalHeaderOffset()
{
    qint64 result=get_lfanew()+4+sizeof(XPE_DEF::S_IMAGE_FILE_HEADER);

    if(!_isOffsetValid(result))
    {
        result=-1;
    }

    return result;
}

XPE_DEF::IMAGE_OPTIONAL_HEADER32 XPE::getOptionalHeader32()
{
    XPE_DEF::IMAGE_OPTIONAL_HEADER32 result= {};

    read_array(getOptionalHeaderOffset(),(char *)&result,sizeof(XPE_DEF::IMAGE_OPTIONAL_HEADER32));

    return result;
}

XPE_DEF::IMAGE_OPTIONAL_HEADER64 XPE::getOptionalHeader64()
{
    XPE_DEF::IMAGE_OPTIONAL_HEADER64 result= {};

    read_array(getOptionalHeaderOffset(),(char *)&result,sizeof(XPE_DEF::IMAGE_OPTIONAL_HEADER64));

    return result;
}

void XPE::setOptionalHeader32(XPE_DEF::IMAGE_OPTIONAL_HEADER32 *pOptionalHeader32)
{
    write_array(getOptionalHeaderOffset(),(char *)pOptionalHeader32,sizeof(XPE_DEF::IMAGE_OPTIONAL_HEADER32));
}

void XPE::setOptionalHeader64(XPE_DEF::IMAGE_OPTIONAL_HEADER64 *pOptionalHeader64)
{
    write_array(getOptionalHeaderOffset(),(char *)pOptionalHeader64,sizeof(XPE_DEF::IMAGE_OPTIONAL_HEADER64));
}

XPE_DEF::IMAGE_OPTIONAL_HEADER32S XPE::getOptionalHeader32S()
{
    XPE_DEF::IMAGE_OPTIONAL_HEADER32S result= {};

    read_array(getOptionalHeaderOffset(),(char *)&result,sizeof(XPE_DEF::IMAGE_OPTIONAL_HEADER32S));

    return result;
}

XPE_DEF::IMAGE_OPTIONAL_HEADER64S XPE::getOptionalHeader64S()
{
    XPE_DEF::IMAGE_OPTIONAL_HEADER64S result= {};

    read_array(getOptionalHeaderOffset(),(char *)&result,sizeof(XPE_DEF::IMAGE_OPTIONAL_HEADER64S));

    return result;
}

void XPE::setOptionalHeader32S(XPE_DEF::IMAGE_OPTIONAL_HEADER32S *pOptionalHeader32S)
{
    // TODO check -1
    write_array(getOptionalHeaderOffset(),(char *)pOptionalHeader32S,sizeof(XPE_DEF::IMAGE_OPTIONAL_HEADER32S));
}

void XPE::setOptionalHeader64S(XPE_DEF::IMAGE_OPTIONAL_HEADER64S *pOptionalHeader64S)
{
    // TODO check -1
    write_array(getOptionalHeaderOffset(),(char *)pOptionalHeader64S,sizeof(XPE_DEF::IMAGE_OPTIONAL_HEADER64S));
}

quint16 XPE::getOptionalHeader_Magic()
{
    return read_uint16(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,Magic));
}

quint8 XPE::getOptionalHeader_MajorLinkerVersion()
{
    return read_uint8(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,MajorLinkerVersion));
}

quint8 XPE::getOptionalHeader_MinorLinkerVersion()
{
    return read_uint8(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,MinorLinkerVersion));
}

quint32 XPE::getOptionalHeader_SizeOfCode()
{
    return read_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,SizeOfCode));
}

quint32 XPE::getOptionalHeader_SizeOfInitializedData()
{
    return read_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,SizeOfInitializedData));
}

quint32 XPE::getOptionalHeader_SizeOfUninitializedData()
{
    return read_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,SizeOfUninitializedData));
}

quint32 XPE::getOptionalHeader_AddressOfEntryPoint()
{
    return read_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,AddressOfEntryPoint));
}

quint32 XPE::getOptionalHeader_BaseOfCode()
{
    return read_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,BaseOfCode));
}

quint32 XPE::getOptionalHeader_BaseOfData()
{
    return read_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,BaseOfData));
}

quint64 XPE::getOptionalHeader_ImageBase()
{
    quint64 nResult=0;

    if(is64())
    {
        nResult=read_uint64(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER64,ImageBase));
    }
    else
    {
        nResult=read_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,ImageBase));
    }

    return nResult;
}

quint32 XPE::getOptionalHeader_SectionAlignment()
{
    return read_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,SectionAlignment));
}

quint32 XPE::getOptionalHeader_FileAlignment()
{
    return read_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,FileAlignment));
}

quint16 XPE::getOptionalHeader_MajorOperatingSystemVersion()
{
    return read_uint16(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,MajorOperatingSystemVersion));
}

quint16 XPE::getOptionalHeader_MinorOperatingSystemVersion()
{
    return read_uint16(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,MinorOperatingSystemVersion));
}

quint16 XPE::getOptionalHeader_MajorImageVersion()
{
    return read_uint16(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,MajorImageVersion));
}

quint16 XPE::getOptionalHeader_MinorImageVersion()
{
    return read_uint16(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,MinorImageVersion));
}

quint16 XPE::getOptionalHeader_MajorSubsystemVersion()
{
    return read_uint16(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,MajorSubsystemVersion));
}

quint16 XPE::getOptionalHeader_MinorSubsystemVersion()
{
    return read_uint16(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,MinorSubsystemVersion));
}

quint32 XPE::getOptionalHeader_Win32VersionValue()
{
    return read_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,Win32VersionValue));
}

quint32 XPE::getOptionalHeader_SizeOfImage()
{
    return read_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,SizeOfImage));
}

quint32 XPE::getOptionalHeader_SizeOfHeaders()
{
    return read_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,SizeOfHeaders));
}

quint32 XPE::getOptionalHeader_CheckSum()
{
    return read_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,CheckSum));
}

quint16 XPE::getOptionalHeader_Subsystem()
{
    return read_uint16(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,Subsystem));
}

quint16 XPE::getOptionalHeader_DllCharacteristics()
{
    return read_uint16(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,DllCharacteristics));
}

qint64 XPE::getOptionalHeader_SizeOfStackReserve()
{
    qint64 nResult=0;

    if(is64())
    {
        nResult=read_uint64(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER64,SizeOfStackReserve));
    }
    else
    {
        nResult=read_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,SizeOfStackReserve));
    }

    return nResult;
}

qint64 XPE::getOptionalHeader_SizeOfStackCommit()
{
    qint64 nResult=0;

    if(is64())
    {
        nResult=read_uint64(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER64,SizeOfStackCommit));
    }
    else
    {
        nResult=read_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,SizeOfStackCommit));
    }

    return nResult;
}

qint64 XPE::getOptionalHeader_SizeOfHeapReserve()
{
    qint64 nResult=0;

    if(is64())
    {
        nResult=read_uint64(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER64,SizeOfHeapReserve));
    }
    else
    {
        nResult=read_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,SizeOfHeapReserve));
    }

    return nResult;
}

qint64 XPE::getOptionalHeader_SizeOfHeapCommit()
{
    qint64 nResult=0;

    if(is64())
    {
        nResult=read_uint64(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER64,SizeOfHeapCommit));
    }
    else
    {
        nResult=read_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,SizeOfHeapCommit));
    }

    return nResult;
}

quint32 XPE::getOptionalHeader_LoaderFlags()
{
    quint32 nResult=0;

    if(is64())
    {
        nResult=read_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER64,LoaderFlags));
    }
    else
    {
        nResult=read_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,LoaderFlags));
    }

    return nResult;
}

quint32 XPE::getOptionalHeader_NumberOfRvaAndSizes()
{
    quint32 nResult=0;

    if(is64())
    {
        nResult=read_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER64,NumberOfRvaAndSizes));
    }
    else
    {
        nResult=read_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,NumberOfRvaAndSizes));
    }

    return nResult;
}

void XPE::setOptionalHeader_Magic(quint16 value)
{
    write_uint16(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,Magic),value);
}

void XPE::setOptionalHeader_MajorLinkerVersion(quint8 value)
{
    write_uint8(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,MajorLinkerVersion),value);
}

void XPE::setOptionalHeader_MinorLinkerVersion(quint8 value)
{
    write_uint8(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,MinorLinkerVersion),value);
}

void XPE::setOptionalHeader_SizeOfCode(quint32 value)
{
    write_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,SizeOfCode),value);
}

void XPE::setOptionalHeader_SizeOfInitializedData(quint32 value)
{
    write_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,SizeOfInitializedData),value);
}

void XPE::setOptionalHeader_SizeOfUninitializedData(quint32 value)
{
    write_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,SizeOfUninitializedData),value);
}

void XPE::setOptionalHeader_AddressOfEntryPoint(quint32 value)
{
    write_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,AddressOfEntryPoint),value);
}

void XPE::setOptionalHeader_BaseOfCode(quint32 value)
{
    write_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,BaseOfCode),value);
}

void XPE::setOptionalHeader_BaseOfData(quint32 value)
{
    // There is no BaseOfData for PE64
    write_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,BaseOfData),value);
}

void XPE::setOptionalHeader_ImageBase(quint64 value)
{
    if(is64())
    {
        write_uint64(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER64,ImageBase),value);
    }
    else
    {
        write_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,ImageBase),value);
    }
}

void XPE::setOptionalHeader_SectionAlignment(quint32 value)
{
    write_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,SectionAlignment),value);
}

void XPE::setOptionalHeader_FileAlignment(quint32 value)
{
    write_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,FileAlignment),value);
}

void XPE::setOptionalHeader_MajorOperatingSystemVersion(quint16 value)
{
    write_uint16(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,MajorOperatingSystemVersion),value);
}

void XPE::setOptionalHeader_MinorOperatingSystemVersion(quint16 value)
{
    write_uint16(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,MinorOperatingSystemVersion),value);
}

void XPE::setOptionalHeader_MajorImageVersion(quint16 value)
{
    write_uint16(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,MajorImageVersion),value);
}

void XPE::setOptionalHeader_MinorImageVersion(quint16 value)
{
    write_uint16(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,MinorImageVersion),value);
}

void XPE::setOptionalHeader_MajorSubsystemVersion(quint16 value)
{
    write_uint16(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,MajorSubsystemVersion),value);
}

void XPE::setOptionalHeader_MinorSubsystemVersion(quint16 value)
{
    write_uint16(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,MinorSubsystemVersion),value);
}

void XPE::setOptionalHeader_Win32VersionValue(quint32 value)
{
    write_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,Win32VersionValue),value);
}

void XPE::setOptionalHeader_SizeOfImage(quint32 value)
{
    write_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,SizeOfImage),value);
}

void XPE::setOptionalHeader_SizeOfHeaders(quint32 value)
{
    write_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,SizeOfHeaders),value);
}

void XPE::setOptionalHeader_CheckSum(quint32 value)
{
    write_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,CheckSum),value);
}

void XPE::setOptionalHeader_Subsystem(quint16 value)
{
    write_uint16(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,Subsystem),value);
}

void XPE::setOptionalHeader_DllCharacteristics(quint16 value)
{
    write_uint16(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,DllCharacteristics),value);
}

void XPE::setOptionalHeader_SizeOfStackReserve(quint64 value)
{
    if(is64())
    {
        write_uint64(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER64,SizeOfStackReserve),value);
    }
    else
    {
        write_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,SizeOfStackReserve),value);
    }
}

void XPE::setOptionalHeader_SizeOfStackCommit(quint64 value)
{
    if(is64())
    {
        write_uint64(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER64,SizeOfStackCommit),value);
    }
    else
    {
        write_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,SizeOfStackCommit),value);
    }
}

void XPE::setOptionalHeader_SizeOfHeapReserve(quint64 value)
{
    if(is64())
    {
        write_uint64(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER64,SizeOfHeapReserve),value);
    }
    else
    {
        write_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,SizeOfHeapReserve),value);
    }
}

void XPE::setOptionalHeader_SizeOfHeapCommit(quint64 value)
{
    if(is64())
    {
        write_uint64(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER64,SizeOfHeapCommit),value);
    }
    else
    {
        write_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,SizeOfHeapCommit),value);
    }
}

void XPE::setOptionalHeader_LoaderFlags(quint32 value)
{
    if(is64())
    {
        write_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER64,LoaderFlags),value);
    }
    else
    {
        write_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,LoaderFlags),value);
    }
}

void XPE::setOptionalHeader_NumberOfRvaAndSizes(quint32 value)
{
    if(is64())
    {
        write_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER64,NumberOfRvaAndSizes),value);
    }
    else
    {
        write_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,NumberOfRvaAndSizes),value);
    }
}

XPE_DEF::IMAGE_DATA_DIRECTORY XPE::getOptionalHeader_DataDirectory(quint32 nNumber)
{
    XPE_DEF::IMAGE_DATA_DIRECTORY result= {};

    //    if(nNumber<getOptionalHeader_NumberOfRvaAndSizes()) // There are some protectors with false NumberOfRvaAndSizes
    if(nNumber<16)
    {
        if(is64())
        {
            read_array(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER64,DataDirectory)+nNumber*sizeof(XPE_DEF::IMAGE_DATA_DIRECTORY),(char *)&result,sizeof(XPE_DEF::IMAGE_DATA_DIRECTORY));
        }
        else
        {
            read_array(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,DataDirectory)+nNumber*sizeof(XPE_DEF::IMAGE_DATA_DIRECTORY),(char *)&result,sizeof(XPE_DEF::IMAGE_DATA_DIRECTORY));
        }
    }

    return result;
}

void XPE::setOptionalHeader_DataDirectory(quint32 nNumber,XPE_DEF::IMAGE_DATA_DIRECTORY *pDataDirectory)
{
    //    if(nNumber<16)
    if(nNumber<getOptionalHeader_NumberOfRvaAndSizes()) // TODO Check!!!
    {
        if(is64())
        {
            write_array(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER64,DataDirectory)+nNumber*sizeof(XPE_DEF::IMAGE_DATA_DIRECTORY),(char *)pDataDirectory,sizeof(XPE_DEF::IMAGE_DATA_DIRECTORY));
        }
        else
        {
            write_array(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,DataDirectory)+nNumber*sizeof(XPE_DEF::IMAGE_DATA_DIRECTORY),(char *)pDataDirectory,sizeof(XPE_DEF::IMAGE_DATA_DIRECTORY));
        }
    }
}

void XPE::setOptionalHeader_DataDirectory_VirtualAddress(quint32 nNumber, quint32 value)
{
    if(nNumber<getOptionalHeader_NumberOfRvaAndSizes())
    {
        if(is64())
        {
            write_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER64,DataDirectory)+nNumber*sizeof(XPE_DEF::IMAGE_DATA_DIRECTORY)+offsetof(XPE_DEF::IMAGE_DATA_DIRECTORY,VirtualAddress),value);
        }
        else
        {
            write_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,DataDirectory)+nNumber*sizeof(XPE_DEF::IMAGE_DATA_DIRECTORY)+offsetof(XPE_DEF::IMAGE_DATA_DIRECTORY,VirtualAddress),value);
        }
    }
}

void XPE::setOptionalHeader_DataDirectory_Size(quint32 nNumber, quint32 value)
{
    if(nNumber<getOptionalHeader_NumberOfRvaAndSizes())
    {
        if(is64())
        {
            write_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER64,DataDirectory)+nNumber*sizeof(XPE_DEF::IMAGE_DATA_DIRECTORY)+offsetof(XPE_DEF::IMAGE_DATA_DIRECTORY,Size),value);
        }
        else
        {
            write_uint32(getOptionalHeaderOffset()+offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,DataDirectory)+nNumber*sizeof(XPE_DEF::IMAGE_DATA_DIRECTORY)+offsetof(XPE_DEF::IMAGE_DATA_DIRECTORY,Size),value);
        }
    }
}

void XPE::clearOptionalHeader_DataDirectory(quint32 nNumber)
{
    XPE_DEF::IMAGE_DATA_DIRECTORY dd= {};
    setOptionalHeader_DataDirectory(nNumber,&dd);
}

bool XPE::isOptionalHeader_DataDirectoryPresent(quint32 nNumber)
{
    XPE_DEF::IMAGE_DATA_DIRECTORY dd=getOptionalHeader_DataDirectory(nNumber);

    //    return (dd.Size)&&(dd.VirtualAddress)&&(isAddressValid(dd.VirtualAddress+getBaseAddress())); // TODO Check
    //    return (dd.Size)&&(dd.VirtualAddress);
    return (dd.VirtualAddress);
}

QList<XPE_DEF::IMAGE_DATA_DIRECTORY> XPE::getDirectories()
{
    QList<XPE_DEF::IMAGE_DATA_DIRECTORY> listResult;

    int nCount=getOptionalHeader_NumberOfRvaAndSizes();
    nCount=qMin(nCount,16);

    qint64 nDirectoriesOffset=getOptionalHeaderOffset();

    if(is64())
    {
        nDirectoriesOffset+=offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER64,DataDirectory);
    }
    else
    {
        nDirectoriesOffset+=offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,DataDirectory);
    }

    for(int i=0; i<nCount; i++)
    {
        XPE_DEF::IMAGE_DATA_DIRECTORY record= {};

        read_array(nDirectoriesOffset+i*sizeof(XPE_DEF::IMAGE_DATA_DIRECTORY),(char *)&record,sizeof(XPE_DEF::IMAGE_DATA_DIRECTORY));

        listResult.append(record);
    }

    return listResult;
}

void XPE::setDirectories(QList<XPE_DEF::IMAGE_DATA_DIRECTORY> *pListDirectories)
{
    int nCount=getOptionalHeader_NumberOfRvaAndSizes();
    nCount=qMin(nCount,16);

    qint64 nDirectoriesOffset=getOptionalHeaderOffset();

    if(is64())
    {
        nDirectoriesOffset+=offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER64,DataDirectory);
    }
    else
    {
        nDirectoriesOffset+=offsetof(XPE_DEF::IMAGE_OPTIONAL_HEADER32,DataDirectory);
    }

    for(int i=0; i<nCount; i++)
    {
        write_array(nDirectoriesOffset+i*sizeof(XPE_DEF::IMAGE_DATA_DIRECTORY),(char *)&(pListDirectories->at(i)),sizeof(XPE_DEF::IMAGE_DATA_DIRECTORY));
    }
}

qint64 XPE::getDataDirectoryOffset(quint32 nNumber)
{
    qint64 nResult=-1;

    XPE_DEF::IMAGE_DATA_DIRECTORY dataResources=getOptionalHeader_DataDirectory(nNumber);

    if(dataResources.VirtualAddress)
    {
        QList<MEMORY_MAP> list=getMemoryMapList();
        qint64 nBaseAddress=_getBaseAddress();

        nResult=addressToOffset(&list,dataResources.VirtualAddress+nBaseAddress);
    }

    return nResult;
}

QByteArray XPE::getDataDirectory(quint32 nNumber)
{
    QByteArray baResult;
    XPE_DEF::IMAGE_DATA_DIRECTORY dataDirectory=getOptionalHeader_DataDirectory(nNumber);

    if(dataDirectory.VirtualAddress)
    {
        QList<MEMORY_MAP> list=getMemoryMapList();
        qint64 nBaseAddress=_getBaseAddress();

        qint64 nOffset=addressToOffset(&list,dataDirectory.VirtualAddress+nBaseAddress);

        if(nOffset!=-1)
        {
            baResult=read_array(nOffset,dataDirectory.Size);
        }
    }

    return baResult;
}

qint64 XPE::getSectionsTableOffset()
{
    qint64 nResult=0;

    nResult=getOptionalHeaderOffset()+getFileHeader_SizeOfOptionalHeader();

    return nResult;
}

XPE_DEF::IMAGE_SECTION_HEADER XPE::getSectionHeader(quint32 nNumber)
{
    XPE_DEF::IMAGE_SECTION_HEADER result= {};
    quint32 nNumberOfSections=getFileHeader_NumberOfSections();

    if(nNumber<nNumberOfSections)
    {
        read_array(getSectionsTableOffset()+nNumber*sizeof(XPE_DEF::IMAGE_SECTION_HEADER),(char *)&result,sizeof(XPE_DEF::IMAGE_SECTION_HEADER));
    }

    return result;
}

void XPE::setSectionHeader(quint32 nNumber, XPE_DEF::IMAGE_SECTION_HEADER *pSectionHeader)
{
    quint32 nNumberOfSections=getFileHeader_NumberOfSections();

    if(nNumber<nNumberOfSections)
    {
        write_array(getSectionsTableOffset()+nNumber*sizeof(XPE_DEF::IMAGE_SECTION_HEADER),(char *)pSectionHeader,sizeof(XPE_DEF::IMAGE_SECTION_HEADER));
    }
}

QList<XPE_DEF::IMAGE_SECTION_HEADER> XPE::getSectionHeaders()
{
    QList<XPE_DEF::IMAGE_SECTION_HEADER> listResult;

    quint32 nNumberOfSections=getFileHeader_NumberOfSections();
    qint64 nSectionOffset=getSectionsTableOffset();

    // Fix
    if(nNumberOfSections>100)
    {
        nNumberOfSections=100;
    }

    for(int i=0; i<(int)nNumberOfSections; i++)
    {
        XPE_DEF::IMAGE_SECTION_HEADER record= {};

        read_array(nSectionOffset+i*sizeof(XPE_DEF::IMAGE_SECTION_HEADER),(char *)&record,sizeof(XPE_DEF::IMAGE_SECTION_HEADER));

        listResult.append(record);
    }

    return listResult;
}

QList<XPE::SECTIONFILE_RECORD> XPE::getSectionRecords(QList<XPE_DEF::IMAGE_SECTION_HEADER> *pList, bool bIsImage)
{
    QList<XPE::SECTIONFILE_RECORD> listResult;

    int nNumberOfSections=pList->count();

    for(int i=0; i<nNumberOfSections; i++)
    {
        XPE::SECTIONFILE_RECORD record= {};

        record.sName=QString((char *)pList->at(i).Name);
        record.sName.resize(qMin(record.sName.length(),XPE_DEF::S_IMAGE_SIZEOF_SHORT_NAME));

        if(bIsImage)
        {
            record.nOffset=pList->at(i).VirtualAddress;
        }
        else
        {
            record.nOffset=pList->at(i).PointerToRawData;
        }

        record.nSize=pList->at(i).SizeOfRawData;
        record.nCharacteristics=pList->at(i).Characteristics;

        listResult.append(record);
    }

    return listResult;
}

QList<XPE::SECTIONRVA_RECORD> XPE::getSectionRVARecords()
{
    QList<SECTIONRVA_RECORD> listResult;

    QList<XPE_DEF::IMAGE_SECTION_HEADER> listSH=getSectionHeaders();
    qint32 nSectionAlignment=getOptionalHeader_SectionAlignment();

    for(int i=0; i<listSH.count(); i++)
    {
        SECTIONRVA_RECORD record= {};

        record.nRVA=listSH.at(i).VirtualAddress;
        record.nSize=S_ALIGN_UP(listSH.at(i).Misc.VirtualSize,nSectionAlignment);
        record.nCharacteristics=listSH.at(i).Characteristics;

        listResult.append(record);
    }

    return listResult;
}

QString XPE::getSection_NameAsString(quint32 nNumber)
{
    QString sResult;
    quint32 nNumberOfSections=getFileHeader_NumberOfSections();

    char cBuffer[9]= {0};

    if(nNumber<nNumberOfSections)
    {
        XBinary::read_array(getSectionsTableOffset()+nNumber*sizeof(XPE_DEF::IMAGE_SECTION_HEADER)+offsetof(XPE_DEF::IMAGE_SECTION_HEADER,Name),cBuffer,8);
    }

    sResult.append(cBuffer);

    return sResult;
}

quint32 XPE::getSection_VirtualSize(quint32 nNumber)
{
    quint32 nResult=0;
    quint32 nNumberOfSections=getFileHeader_NumberOfSections();

    if(nNumber<nNumberOfSections)
    {
        nResult=read_uint32(getSectionsTableOffset()+nNumber*sizeof(XPE_DEF::IMAGE_SECTION_HEADER)+offsetof(XPE_DEF::IMAGE_SECTION_HEADER,Misc.VirtualSize));
    }

    return nResult;
}

quint32 XPE::getSection_VirtualAddress(quint32 nNumber)
{
    quint32 nResult=0;
    quint32 nNumberOfSections=getFileHeader_NumberOfSections();

    if(nNumber<nNumberOfSections)
    {
        nResult=read_uint32(getSectionsTableOffset()+nNumber*sizeof(XPE_DEF::IMAGE_SECTION_HEADER)+offsetof(XPE_DEF::IMAGE_SECTION_HEADER,VirtualAddress));
    }

    return nResult;
}

quint32 XPE::getSection_SizeOfRawData(quint32 nNumber)
{
    quint32 nResult=0;
    quint32 nNumberOfSections=getFileHeader_NumberOfSections();

    if(nNumber<nNumberOfSections)
    {
        nResult=read_uint32(getSectionsTableOffset()+nNumber*sizeof(XPE_DEF::IMAGE_SECTION_HEADER)+offsetof(XPE_DEF::IMAGE_SECTION_HEADER,SizeOfRawData));
    }

    return nResult;
}

quint32 XPE::getSection_PointerToRawData(quint32 nNumber)
{
    quint32 nResult=0;
    quint32 nNumberOfSections=getFileHeader_NumberOfSections();

    if(nNumber<nNumberOfSections)
    {
        nResult=read_uint32(getSectionsTableOffset()+nNumber*sizeof(XPE_DEF::IMAGE_SECTION_HEADER)+offsetof(XPE_DEF::IMAGE_SECTION_HEADER,PointerToRawData));
    }

    return nResult;
}

quint32 XPE::getSection_PointerToRelocations(quint32 nNumber)
{
    quint32 nResult=0;
    quint32 nNumberOfSections=getFileHeader_NumberOfSections();

    if(nNumber<nNumberOfSections)
    {
        nResult=read_uint32(getSectionsTableOffset()+nNumber*sizeof(XPE_DEF::IMAGE_SECTION_HEADER)+offsetof(XPE_DEF::IMAGE_SECTION_HEADER,PointerToRelocations));
    }

    return nResult;
}

quint32 XPE::getSection_PointerToLinenumbers(quint32 nNumber)
{
    quint32 nResult=0;
    quint32 nNumberOfSections=getFileHeader_NumberOfSections();

    if(nNumber<nNumberOfSections)
    {
        nResult=read_uint32(getSectionsTableOffset()+nNumber*sizeof(XPE_DEF::IMAGE_SECTION_HEADER)+offsetof(XPE_DEF::IMAGE_SECTION_HEADER,PointerToLinenumbers));
    }

    return nResult;
}

quint16 XPE::getSection_NumberOfRelocations(quint32 nNumber)
{
    quint16 nResult=0;
    quint32 nNumberOfSections=getFileHeader_NumberOfSections();

    if(nNumber<nNumberOfSections)
    {
        nResult=read_uint16(getSectionsTableOffset()+nNumber*sizeof(XPE_DEF::IMAGE_SECTION_HEADER)+offsetof(XPE_DEF::IMAGE_SECTION_HEADER,NumberOfRelocations));
    }

    return nResult;
}

quint16 XPE::getSection_NumberOfLinenumbers(quint32 nNumber)
{
    quint16 nResult=0;
    quint32 nNumberOfSections=getFileHeader_NumberOfSections();

    if(nNumber<nNumberOfSections)
    {
        nResult=read_uint16(getSectionsTableOffset()+nNumber*sizeof(XPE_DEF::IMAGE_SECTION_HEADER)+offsetof(XPE_DEF::IMAGE_SECTION_HEADER,NumberOfLinenumbers));
    }

    return nResult;
}

quint32 XPE::getSection_Characteristics(quint32 nNumber)
{
    quint32 nResult=0;
    quint32 nNumberOfSections=getFileHeader_NumberOfSections();

    if(nNumber<nNumberOfSections)
    {
        nResult=read_uint32(getSectionsTableOffset()+nNumber*sizeof(XPE_DEF::IMAGE_SECTION_HEADER)+offsetof(XPE_DEF::IMAGE_SECTION_HEADER,Characteristics));
    }

    return nResult;
}

void XPE::setSection_NameAsString(quint32 nNumber, QString sName)
{
    quint32 nNumberOfSections=getFileHeader_NumberOfSections();

    char cBuffer[9]= {0};

    sName.resize(8);

    strcpy(cBuffer,sName.toLatin1().data());

    if(nNumber<nNumberOfSections)
    {
        XBinary::write_array(getSectionsTableOffset()+nNumber*sizeof(XPE_DEF::IMAGE_SECTION_HEADER)+offsetof(XPE_DEF::IMAGE_SECTION_HEADER,Name),cBuffer,8);
    }
}

void XPE::setSection_VirtualSize(quint32 nNumber, quint32 value)
{
    quint32 nNumberOfSections=getFileHeader_NumberOfSections();

    if(nNumber<nNumberOfSections)
    {
        write_uint32(getSectionsTableOffset()+nNumber*sizeof(XPE_DEF::IMAGE_SECTION_HEADER)+offsetof(XPE_DEF::IMAGE_SECTION_HEADER,Misc.VirtualSize),value);
    }
}

void XPE::setSection_VirtualAddress(quint32 nNumber, quint32 value)
{
    quint32 nNumberOfSections=getFileHeader_NumberOfSections();

    if(nNumber<nNumberOfSections)
    {
        write_uint32(getSectionsTableOffset()+nNumber*sizeof(XPE_DEF::IMAGE_SECTION_HEADER)+offsetof(XPE_DEF::IMAGE_SECTION_HEADER,VirtualAddress),value);
    }
}

void XPE::setSection_SizeOfRawData(quint32 nNumber, quint32 value)
{
    quint32 nNumberOfSections=getFileHeader_NumberOfSections();

    if(nNumber<nNumberOfSections)
    {
        write_uint32(getSectionsTableOffset()+nNumber*sizeof(XPE_DEF::IMAGE_SECTION_HEADER)+offsetof(XPE_DEF::IMAGE_SECTION_HEADER,SizeOfRawData),value);
    }
}

void XPE::setSection_PointerToRawData(quint32 nNumber, quint32 value)
{
    quint32 nNumberOfSections=getFileHeader_NumberOfSections();

    if(nNumber<nNumberOfSections)
    {
        write_uint32(getSectionsTableOffset()+nNumber*sizeof(XPE_DEF::IMAGE_SECTION_HEADER)+offsetof(XPE_DEF::IMAGE_SECTION_HEADER,PointerToRawData),value);
    }
}

void XPE::setSection_PointerToRelocations(quint32 nNumber, quint32 value)
{
    quint32 nNumberOfSections=getFileHeader_NumberOfSections();

    if(nNumber<nNumberOfSections)
    {
        write_uint32(getSectionsTableOffset()+nNumber*sizeof(XPE_DEF::IMAGE_SECTION_HEADER)+offsetof(XPE_DEF::IMAGE_SECTION_HEADER,PointerToRelocations),value);
    }
}

void XPE::setSection_PointerToLinenumbers(quint32 nNumber, quint32 value)
{
    quint32 nNumberOfSections=getFileHeader_NumberOfSections();

    if(nNumber<nNumberOfSections)
    {
        write_uint32(getSectionsTableOffset()+nNumber*sizeof(XPE_DEF::IMAGE_SECTION_HEADER)+offsetof(XPE_DEF::IMAGE_SECTION_HEADER,PointerToLinenumbers),value);
    }
}

void XPE::setSection_NumberOfRelocations(quint32 nNumber, quint16 value)
{
    quint32 nNumberOfSections=getFileHeader_NumberOfSections();

    if(nNumber<nNumberOfSections)
    {
        write_uint16(getSectionsTableOffset()+nNumber*sizeof(XPE_DEF::IMAGE_SECTION_HEADER)+offsetof(XPE_DEF::IMAGE_SECTION_HEADER,NumberOfRelocations),value);
    }
}

void XPE::setSection_NumberOfLinenumbers(quint32 nNumber, quint16 value)
{
    quint32 nNumberOfSections=getFileHeader_NumberOfSections();

    if(nNumber<nNumberOfSections)
    {
        write_uint16(getSectionsTableOffset()+nNumber*sizeof(XPE_DEF::IMAGE_SECTION_HEADER)+offsetof(XPE_DEF::IMAGE_SECTION_HEADER,NumberOfLinenumbers),value);
    }
}

void XPE::setSection_Characteristics(quint32 nNumber, quint32 value)
{
    quint32 nNumberOfSections=getFileHeader_NumberOfSections();

    if(nNumber<nNumberOfSections)
    {
        write_uint32(getSectionsTableOffset()+nNumber*sizeof(XPE_DEF::IMAGE_SECTION_HEADER)+offsetof(XPE_DEF::IMAGE_SECTION_HEADER,Characteristics),value);
    }
}

bool XPE::isSectionNamePresent(QString sSectionName, QList<XPE_DEF::IMAGE_SECTION_HEADER> *pListSections)
{
    bool bResult=false;

    int nNumberOfSections=pListSections->count();

    for(int i=0; i<nNumberOfSections; i++)
    {
        QString _sSectionName=QString((char *)pListSections->at(i).Name);
        _sSectionName.resize(qMin(_sSectionName.length(),XPE_DEF::S_IMAGE_SIZEOF_SHORT_NAME));

        if(_sSectionName==sSectionName)
        {
            bResult=true;
            break;
        }
    }

    return bResult;
}

XPE_DEF::IMAGE_SECTION_HEADER XPE::getSectionByName(QString sSectionName, QList<XPE_DEF::IMAGE_SECTION_HEADER> *pListSections)
{
    XPE_DEF::IMAGE_SECTION_HEADER result= {};

    int nNumberOfSections=pListSections->count();

    for(int i=0; i<nNumberOfSections; i++)
    {
        QString _sSectionName=QString((char *)pListSections->at(i).Name);
        _sSectionName.resize(qMin(_sSectionName.length(),XPE_DEF::S_IMAGE_SIZEOF_SHORT_NAME));

        if(_sSectionName==sSectionName)
        {
            result=pListSections->at(i);

            break;
        }
    }

    return result;
}

bool XPE::isImportPresent()
{
    return isOptionalHeader_DataDirectoryPresent(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_IMPORT);
}

QList<XBinary::MEMORY_MAP> XPE::getMemoryMapList()
{
    QList<MEMORY_MAP> list;

    quint32 nNumberOfSections=qMin((int)getFileHeader_NumberOfSections(),100);
    quint32 nFileAlignment=getOptionalHeader_FileAlignment();
    quint32 nSectionAlignment=getOptionalHeader_SectionAlignment();
    //qint64 nBaseAddress=getOptionalHeader_ImageBase();
    qint64 nBaseAddress=_getBaseAddress();
    quint32 nHeadersSize=getOptionalHeader_SizeOfHeaders(); // mb TODO calc for UPX

    if(nFileAlignment==nSectionAlignment)
    {
        nFileAlignment=1;
    }

    quint32 nVirtualSizeofHeaders=S_ALIGN_UP(nHeadersSize,nSectionAlignment);
    qint64 nMaxOffset=0;

    MEMORY_MAP recordHeaderRaw= {};

    if(!isImage())
    {
        recordHeaderRaw.bIsHeader=true;
        recordHeaderRaw.nAddress=nBaseAddress;
        recordHeaderRaw.segment=ADDRESS_SEGMENT_FLAT;
        recordHeaderRaw.nOffset=0;
        recordHeaderRaw.nSize=nHeadersSize;

        list.append(recordHeaderRaw);

        if(nVirtualSizeofHeaders-nHeadersSize)
        {
            MEMORY_MAP record= {};
            record.bIsHeader=true;

            record.nAddress=nBaseAddress+nHeadersSize;
            recordHeaderRaw.segment=ADDRESS_SEGMENT_FLAT;
            record.nOffset=-1;
            record.nSize=nVirtualSizeofHeaders-nHeadersSize;

            list.append(record);
        }
    }
    else
    {
        recordHeaderRaw.bIsHeader=true;
        recordHeaderRaw.nAddress=nBaseAddress;
        recordHeaderRaw.segment=ADDRESS_SEGMENT_FLAT;
        recordHeaderRaw.nOffset=0;
        recordHeaderRaw.nSize=nVirtualSizeofHeaders;

        list.append(recordHeaderRaw);
    }

    nMaxOffset=recordHeaderRaw.nSize;

    for(quint32 i=0; i<nNumberOfSections; i++)
    {
        XPE_DEF::IMAGE_SECTION_HEADER section=getSectionHeader(i);
        // TODO for corrupted files
        qint64 nFileOffset=section.PointerToRawData;
        //
        nFileOffset=S_ALIGN_DOWN(nFileOffset,nFileAlignment);
        //        qint64 nFileSize=__ALIGN_UP(section.SizeOfRawData,nFileAlignment);
        qint64 nFileSize=section.SizeOfRawData+(section.PointerToRawData-nFileOffset);
        qint64 nVirtualAddress=nBaseAddress+section.VirtualAddress;
        qint64 nVirtualSize=S_ALIGN_UP(section.Misc.VirtualSize,nSectionAlignment);

        if(!isImage())
        {
            nMaxOffset=qMax(nMaxOffset,(qint64)(nFileOffset+nFileSize));
        }
        else
        {
            nMaxOffset=qMax(nMaxOffset,(qint64)(nVirtualAddress+nVirtualSize));
        }

        if(!isImage())
        {
            if(nFileSize)
            {
                MEMORY_MAP record= {};

                record.bIsLoadSection=true;
                record.nLoadSection=i;
                record.segment=ADDRESS_SEGMENT_FLAT;
                record.nAddress=nVirtualAddress;
                record.nOffset=nFileOffset;
                record.nSize=nFileSize;

                list.append(record);
            }

            if(nVirtualSize-nFileSize)
            {
                MEMORY_MAP record= {};

                record.bIsLoadSection=true;
                record.nLoadSection=i;
                record.segment=ADDRESS_SEGMENT_FLAT;
                record.nAddress=nVirtualAddress+nFileSize;
                record.nOffset=-1;
                record.nSize=nVirtualSize-nFileSize;

                list.append(record);
            }
        }
        else
        {
            MEMORY_MAP record= {};

            record.bIsLoadSection=true;
            record.nLoadSection=i;
            record.segment=ADDRESS_SEGMENT_FLAT;
            record.nAddress=nVirtualAddress;
            record.nOffset=nVirtualAddress-nBaseAddress;
            record.nSize=nVirtualSize;

            list.append(record);
        }
    }

    // Overlay;
    MEMORY_MAP record= {};

    record.bIsOvelay=true;

    record.nAddress=-1;
    record.segment=ADDRESS_SEGMENT_UNKNOWN;
    record.nOffset=nMaxOffset;
    record.nSize=0;

    if(!isImage())
    {
        record.nSize=qMax(getSize()-nMaxOffset,(qint64)0);
    }

    list.append(record);

    return list;
}

qint64 XPE::getBaseAddress()
{
    return (qint64)getOptionalHeader_ImageBase();
}

void XPE::setBaseAddress(qint64 nBaseAddress)
{
    setOptionalHeader_ImageBase(nBaseAddress);
}

qint64 XPE::getEntryPointOffset()
{
    return addressToOffset(_getBaseAddress()+getOptionalHeader_AddressOfEntryPoint());
}

void XPE::setEntryPointOffset(qint64 nEntryPointOffset)
{
    setOptionalHeader_AddressOfEntryPoint(offsetToAddress(nEntryPointOffset)-_getBaseAddress());
}

QList<XPE::IMPORT_RECORD> XPE::getImportRecords()
{
    QList<IMPORT_RECORD> listResult;

    qint64 nImportOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_IMPORT);

    if(nImportOffset!=-1)
    {
        QList<MEMORY_MAP> listMemoryMap=getMemoryMapList();
        qint64 nBaseAddress=_getBaseAddress();
        bool bIs64=is64();

        while(true)
        {
            XPE_DEF::IMAGE_IMPORT_DESCRIPTOR iid=read_IMAGE_IMPORT_DESCRIPTOR(nImportOffset);

            QString sLibrary;

            if((iid.Characteristics==0)&&(iid.Name==0))
            {
                break;
            }

            qint64 nOffset=addressToOffset(&listMemoryMap,iid.Name+nBaseAddress);

            if(nOffset!=-1)
            {
                sLibrary=read_ansiString(nOffset);

                if(sLibrary=="")
                {
                    break;
                }
            }
            else
            {
                break; // corrupted
            }

            qint64 nThunksOffset=-1;
            qint64 nRVA=0;

            if(iid.OriginalFirstThunk)
            {
                nThunksOffset=addressToOffset(&listMemoryMap,iid.OriginalFirstThunk+nBaseAddress);
                //                nRVA=iid.OriginalFirstThunk;
            }
            else if((iid.FirstThunk))
            {
                nThunksOffset=addressToOffset(&listMemoryMap,iid.FirstThunk+nBaseAddress);
                //                nRVA=iid.FirstThunk;
            }

            nRVA=iid.FirstThunk;

            if(nThunksOffset==-1)
            {
                break;
            }

            while(true)
            {
                QString sFunction;

                if(bIs64)
                {
                    qint64 nThunk64=read_uint64(nThunksOffset);

                    if(nThunk64==0)
                    {
                        break;
                    }

                    if(!(nThunk64&0x8000000000000000))
                    {
                        qint64 nOffset=addressToOffset(&listMemoryMap,nThunk64+nBaseAddress);

                        if(nOffset!=-1)
                        {
                            sFunction=read_ansiString(nOffset+2);

                            if(sFunction=="")
                            {
                                break;
                            }
                        }
                        else
                        {
                            break;
                        }
                    }
                    else
                    {
                        sFunction=QString("%1").arg(nThunk64&0x7FFFFFFFFFFFFFFF);
                    }
                }
                else
                {
                    qint64 nThunk32=read_uint32(nThunksOffset);

                    if(nThunk32==0)
                    {
                        break;
                    }

                    if(!(nThunk32&0x80000000))
                    {
                        qint64 nOffset=addressToOffset(&listMemoryMap,nThunk32+nBaseAddress);

                        if(nOffset!=-1)
                        {
                            sFunction=read_ansiString(nOffset+2);

                            if(sFunction=="")
                            {
                                break;
                            }
                        }
                        else
                        {
                            break;
                        }
                    }
                    else
                    {
                        sFunction=QString("%1").arg(nThunk32&0x7FFFFFFF);
                    }
                }

                IMPORT_RECORD record;

                record.nOffset=nThunksOffset;
                record.nRVA=nRVA;
                record.sLibrary=sLibrary;
                record.sFunction=sFunction;

                listResult.append(record);

                if(bIs64)
                {
                    nThunksOffset+=8;
                    nRVA+=8;
                }
                else
                {
                    nThunksOffset+=4;
                    nRVA+=4;
                }
            }

            nImportOffset+=sizeof(XPE_DEF::IMAGE_IMPORT_DESCRIPTOR);
        }
    }

    return listResult;
}

QList<XPE_DEF::IMAGE_IMPORT_DESCRIPTOR> XPE::getImportDescriptors()
{
    QList<XPE_DEF::IMAGE_IMPORT_DESCRIPTOR> listResult;

    qint64 nImportOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_IMPORT);

    if(nImportOffset!=-1)
    {
        QList<MEMORY_MAP> listMemoryMap=getMemoryMapList();
        qint64 nBaseAddress=_getBaseAddress();

        while(true)
        {
            XPE_DEF::IMAGE_IMPORT_DESCRIPTOR iid=read_IMAGE_IMPORT_DESCRIPTOR(nImportOffset);

            if((iid.Characteristics==0)&&(iid.Name==0))
            {
                break;
            }

            qint64 nOffset=addressToOffset(&listMemoryMap,iid.Name+nBaseAddress);

            if(nOffset!=-1)
            {
                QString sName=read_ansiString(nOffset);

                if(sName=="")
                {
                    break;
                }
            }
            else
            {
                break; // corrupted
            }

            listResult.append(iid);

            nImportOffset+=sizeof(XPE_DEF::IMAGE_IMPORT_DESCRIPTOR);
        }
    }

    return listResult;
}

QList<XPE::IMAGE_IMPORT_DESCRIPTOR_EX> XPE::getImportDescriptorsEx()
{
    QList<IMAGE_IMPORT_DESCRIPTOR_EX> listResult;

    qint64 nImportOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_IMPORT);

    if(nImportOffset!=-1)
    {
        QList<MEMORY_MAP> listMemoryMap=getMemoryMapList();
        qint64 nBaseAddress=_getBaseAddress();
        //        bool bIs64=is64();

        while(true)
        {
            IMAGE_IMPORT_DESCRIPTOR_EX record= {};
            XPE_DEF::IMAGE_IMPORT_DESCRIPTOR iid=read_IMAGE_IMPORT_DESCRIPTOR(nImportOffset);

            if((iid.Characteristics==0)&&(iid.Name==0))
            {
                break;
            }

            qint64 nOffset=addressToOffset(&listMemoryMap,iid.Name+nBaseAddress);

            if(nOffset!=-1)
            {
                record.sLibrary=read_ansiString(nOffset);

                if(record.sLibrary=="")
                {
                    break;
                }
            }
            else
            {
                break; // corrupted
            }

            record.Characteristics=iid.Characteristics;
            record.FirstThunk=iid.FirstThunk;
            record.ForwarderChain=iid.ForwarderChain;
            record.Name=iid.Name;
            record.OriginalFirstThunk=iid.OriginalFirstThunk;
            record.TimeDateStamp=iid.TimeDateStamp;

            listResult.append(record);

            nImportOffset+=sizeof(XPE_DEF::IMAGE_IMPORT_DESCRIPTOR);
        }
    }

    return listResult;
}

XPE_DEF::IMAGE_IMPORT_DESCRIPTOR XPE::getImportDescriptor(quint32 nNumber)
{
    XPE_DEF::IMAGE_IMPORT_DESCRIPTOR result= {};

    qint64 nImportOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_IMPORT);

    if(nImportOffset!=-1)
    {
        nImportOffset+=nNumber*sizeof(XPE_DEF::IMAGE_IMPORT_DESCRIPTOR);

        result=read_IMAGE_IMPORT_DESCRIPTOR(nImportOffset);
    }

    return result;
}

void XPE::setImportDescriptor(quint32 nNumber, XPE_DEF::IMAGE_IMPORT_DESCRIPTOR *pImportDescriptor)
{
    qint64 nImportOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_IMPORT);

    if(nImportOffset!=-1)
    {
        nImportOffset+=nNumber*sizeof(XPE_DEF::IMAGE_IMPORT_DESCRIPTOR);

        write_IMAGE_IMPORT_DESCRIPTOR(nImportOffset,*pImportDescriptor);
    }
}
// TODO: function with QList<MEMORY_MAP>
QList<XPE::IMPORT_HEADER> XPE::getImports()
{
    QList<IMPORT_HEADER> listResult;

    XPE_DEF::IMAGE_DATA_DIRECTORY dataResources=getOptionalHeader_DataDirectory(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_IMPORT);
    QList<MEMORY_MAP> listMemoryMap=getMemoryMapList();
    qint64 nBaseAddress=_getBaseAddress();
    qint64 nImportOffset=-1;
    qint64 nImportOffsetTest=-1;

    if(dataResources.VirtualAddress)
    {
        nImportOffset=addressToOffset(&listMemoryMap,dataResources.VirtualAddress+nBaseAddress);
        nImportOffsetTest=addressToOffset(&listMemoryMap,dataResources.VirtualAddress+nBaseAddress+sizeof(XPE_DEF::IMAGE_IMPORT_DESCRIPTOR)-2); // Test for some (Win)Upack stubs
    }

    if(nImportOffset!=-1)
    {
        bool bIs64=is64();

        while(true)
        {
            XPE_DEF::IMAGE_IMPORT_DESCRIPTOR iid=read_IMAGE_IMPORT_DESCRIPTOR(nImportOffset);

            IMPORT_HEADER importHeader= {};

            if(nImportOffsetTest==-1)
            {
                iid.FirstThunk&=0x0000FFFF;
            }

            if((iid.Characteristics==0)&&(iid.Name==0))
            {
                break;
            }

            qint64 nOffset=addressToOffset(&listMemoryMap,iid.Name+nBaseAddress);

            if(nOffset!=-1)
            {
                importHeader.sName=read_ansiString(nOffset);

                if(importHeader.sName=="")
                {
                    break;
                }
            }
            else
            {
                break; // corrupted
            }

            qint64 nThunksOffset=-1;
            qint64 nThunksRVA=0;
            qint64 nThunksOriginalRVA=0;
            qint64 nThunksOriginalOffset=0;

            if(iid.OriginalFirstThunk)
            {
                nThunksRVA=iid.OriginalFirstThunk;
                //                nRVA=iid.OriginalFirstThunk;
            }
            else if((iid.FirstThunk))
            {
                nThunksRVA=iid.FirstThunk;
                //                nRVA=iid.FirstThunk;
            }

            nThunksOriginalRVA=iid.FirstThunk;

            nThunksOffset=addressToOffset(&listMemoryMap,nThunksRVA+nBaseAddress);
            nThunksOriginalOffset=addressToOffset(&listMemoryMap,nThunksOriginalRVA+nBaseAddress);

            if(nThunksOffset==-1)
            {
                break;
            }

            while(true)
            {
                IMPORT_POSITION importPosition= {};
                importPosition.nThunkRVA=nThunksOriginalRVA;
                importPosition.nThunkOffset=nThunksOriginalOffset;

                if(bIs64)
                {
                    importPosition.nThunkValue=read_uint64(nThunksOffset);

                    if(importPosition.nThunkValue==0)
                    {
                        break;
                    }

                    if(!(importPosition.nThunkValue&0x8000000000000000))
                    {
                        qint64 nOffset=addressToOffset(&listMemoryMap,importPosition.nThunkValue+nBaseAddress);

                        if(nOffset!=-1)
                        {
                            importPosition.nHint=read_uint16(nOffset);
                            importPosition.sName=read_ansiString(nOffset+2);

                            if(importPosition.sName=="")
                            {
                                break;
                            }
                        }
                        else
                        {
                            break;
                        }
                    }
                    else
                    {
                        importPosition.nOrdinal=importPosition.nThunkValue&0x7FFFFFFFFFFFFFFF;
                    }

                }
                else
                {
                    importPosition.nThunkValue=read_uint32(nThunksOffset);

                    if(importPosition.nThunkValue==0)
                    {
                        break;
                    }

                    if(!(importPosition.nThunkValue&0x80000000))
                    {
                        qint64 nOffset=addressToOffset(&listMemoryMap,importPosition.nThunkValue+nBaseAddress);

                        if(nOffset!=-1)
                        {
                            importPosition.nHint=read_uint16(nOffset);
                            importPosition.sName=read_ansiString(nOffset+2);

                            if(importPosition.sName=="")
                            {
                                break;
                            }
                        }
                        else
                        {
                            break;
                        }
                    }
                    else
                    {
                        importPosition.nOrdinal=importPosition.nThunkValue&0x7FFFFFFF;
                    }
                }

                if(importPosition.nOrdinal==0)
                {
                    importPosition.sFunction=importPosition.sName;
                }
                else
                {
                    importPosition.sFunction=QString("%1").arg(importPosition.nOrdinal);
                }

                if(bIs64)
                {
                    nThunksOffset+=8;
                    nThunksRVA+=8;
                    nThunksOriginalRVA+=8;
                    nThunksOriginalOffset+=8;
                }
                else
                {
                    nThunksOffset+=4;
                    nThunksRVA+=4;
                    nThunksOriginalRVA+=4;
                    nThunksOriginalOffset+=4;
                }

                importHeader.listPositions.append(importPosition);
            }

            listResult.append(importHeader);

            nImportOffset+=sizeof(XPE_DEF::IMAGE_IMPORT_DESCRIPTOR);
        }
    }

    return listResult;
}

QList<XPE::IMPORT_POSITION> XPE::getImportPositions(int nIndex)
{
    QList<IMPORT_POSITION> listResult;

    qint64 nImportOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_IMPORT);

    if(nImportOffset!=-1)
    {
        QList<MEMORY_MAP> listMemoryMap=getMemoryMapList();
        qint64 nBaseAddress=_getBaseAddress();
        bool bIs64=is64();

        int _nIndex=0;

        while(true)
        {
            IMPORT_HEADER importHeader= {};
            XPE_DEF::IMAGE_IMPORT_DESCRIPTOR iid=read_IMAGE_IMPORT_DESCRIPTOR(nImportOffset);

            if((iid.Characteristics==0)&&(iid.Name==0))
            {
                break;
            }

            qint64 nOffset=addressToOffset(&listMemoryMap,iid.Name+nBaseAddress);

            if(nOffset!=-1)
            {
                importHeader.sName=read_ansiString(nOffset);

                if(importHeader.sName=="")
                {
                    break;
                }
            }
            else
            {
                break; // corrupted
            }

            qint64 nThunksOffset=-1;
            qint64 nRVA=0;
            qint64 nThunksRVA=-1;

            if(iid.OriginalFirstThunk)
            {
                nThunksRVA=iid.OriginalFirstThunk+nBaseAddress;
                nThunksOffset=addressToOffset(&listMemoryMap,nThunksRVA);
                //                nRVA=iid.OriginalFirstThunk;
            }
            else if((iid.FirstThunk))
            {
                nThunksRVA=iid.FirstThunk+nBaseAddress;
                nThunksOffset=addressToOffset(&listMemoryMap,nThunksRVA);
                //                nRVA=iid.FirstThunk;
            }

            nRVA=iid.FirstThunk;

            if(nThunksOffset==-1)
            {
                break;
            }

            if(_nIndex==nIndex)
            {
                while(true)
                {
                    IMPORT_POSITION importPosition= {};
                    importPosition.nThunkOffset=nThunksOffset;
                    importPosition.nThunkRVA=nThunksRVA;

                    if(bIs64)
                    {
                        importPosition.nThunkValue=read_uint64(nThunksOffset);

                        if(importPosition.nThunkValue==0)
                        {
                            break;
                        }

                        if(!(importPosition.nThunkValue&0x8000000000000000))
                        {
                            qint64 nOffset=addressToOffset(&listMemoryMap,importPosition.nThunkValue+nBaseAddress);

                            if(nOffset!=-1)
                            {
                                importPosition.nHint=read_uint16(nOffset);
                                importPosition.sName=read_ansiString(nOffset+2);

                                if(importPosition.sName=="")
                                {
                                    break;
                                }
                            }
                            else
                            {
                                break;
                            }
                        }
                        else
                        {
                            importPosition.nOrdinal=importPosition.nThunkValue&0x7FFFFFFFFFFFFFFF;
                        }

                    }
                    else
                    {
                        importPosition.nThunkValue=read_uint32(nThunksOffset);

                        if(importPosition.nThunkValue==0)
                        {
                            break;
                        }

                        if(!(importPosition.nThunkValue&0x80000000))
                        {
                            qint64 nOffset=addressToOffset(&listMemoryMap,importPosition.nThunkValue+nBaseAddress);

                            if(nOffset!=-1)
                            {
                                importPosition.nHint=read_uint16(nOffset);
                                importPosition.sName=read_ansiString(nOffset+2);

                                if(importPosition.sName=="")
                                {
                                    break;
                                }
                            }
                            else
                            {
                                break;
                            }
                        }
                        else
                        {
                            importPosition.nOrdinal=importPosition.nThunkValue&0x7FFFFFFF;
                        }
                    }

                    if(bIs64)
                    {
                        nThunksRVA+=8;
                        nThunksOffset+=8;
                        nRVA+=8;
                    }
                    else
                    {
                        nThunksRVA+=8;
                        nThunksOffset+=4;
                        nRVA+=4;
                    }

                    listResult.append(importPosition);
                }

                break;
            }

            nImportOffset+=sizeof(XPE_DEF::IMAGE_IMPORT_DESCRIPTOR);
            _nIndex++;
        }
    }

    return listResult;
}

bool XPE::isImportLibraryPresentI(QString sLibrary, QList<XPE::IMPORT_HEADER> *pListImport)
{
    bool bResult=false;

    for(int i=0; i<pListImport->count(); i++)
    {
        if(pListImport->at(i).sName.toUpper()==sLibrary.toUpper())
        {
            bResult=true;
            break;
        }
    }

    return bResult;
}

bool XPE::isImportFunctionPresentI(QString sLibrary, QString sFunction, QList<XPE::IMPORT_HEADER> *pListImport)
{
    bool bResult=false;

    for(int i=0; i<pListImport->count(); i++)
    {
        if(pListImport->at(i).sName.toUpper()==sLibrary.toUpper())
        {
            for(int j=0;j<pListImport->at(i).listPositions.count();j++)
            {
                if(pListImport->at(i).listPositions.at(j).sFunction==sFunction)
                {
                    bResult=true;
                    break;
                }
            }
        }
    }

    return bResult;
}

bool XPE::setImports(QList<XPE::IMPORT_HEADER> *pListHeaders)
{
    return setImports(getDevice(),isImage(),pListHeaders);
}

bool XPE::setImports(QIODevice *pDevice,bool bIsImage, QList<XPE::IMPORT_HEADER> *pListHeaders)
{
    bool bResult=false;

    if(isResizeEnable(pDevice))
    {
        XPE pe(pDevice,bIsImage);

        if(pe.isValid())
        {
            int nAddressSize=4;

            if(pe.is64())
            {
                nAddressSize=8;
            }
            else
            {
                nAddressSize=4;
            }

            QByteArray baImport;
            QList<qint64> listPatches; // Addresses for patch
            //    QMap<qint64,qint64> mapMove;

            // Calculate
            quint32 nIATSize=0;
            quint32 nImportTableSize=(pListHeaders->count()+1)*sizeof(XPE_DEF::IMAGE_IMPORT_DESCRIPTOR);
            quint32 nAnsiDataSize=0;

            for(int i=0; i<pListHeaders->count(); i++)
            {
                // TODO 64
                nIATSize+=(pListHeaders->at(i).listPositions.count()+1)*nAddressSize;
                nAnsiDataSize+=pListHeaders->at(i).sName.length()+3;

                for(int j=0; j<pListHeaders->at(i).listPositions.count(); j++)
                {
                    if(pListHeaders->at(i).listPositions.at(j).sName!="")
                    {
                        nAnsiDataSize+=2+pListHeaders->at(i).listPositions.at(j).sName.length()+1;
                    }
                }
            }

            nImportTableSize=S_ALIGN_UP(nImportTableSize,16);
            nIATSize=S_ALIGN_UP(nIATSize,16);
            nAnsiDataSize=S_ALIGN_UP(nAnsiDataSize,16);

            baImport.resize(nIATSize+nImportTableSize+nIATSize+nAnsiDataSize);
            baImport.fill(0);

            char *pDataOffset=baImport.data();
            char *pIAT=pDataOffset;
            XPE_DEF::IMAGE_IMPORT_DESCRIPTOR *pIID=(XPE_DEF::IMAGE_IMPORT_DESCRIPTOR *)(pDataOffset+nIATSize);
            char *pOIAT=pDataOffset+nIATSize+nImportTableSize;
            char *pAnsiData=pDataOffset+nIATSize+nImportTableSize+nIATSize;

            for(int i=0; i<pListHeaders->count(); i++)
            {
                pIID->FirstThunk=pIAT-pDataOffset;
                listPatches.append((char *)pIID-pDataOffset+offsetof(XPE_DEF::IMAGE_IMPORT_DESCRIPTOR,FirstThunk));

                pIID->Name=pAnsiData-pDataOffset;
                listPatches.append((char *)pIID-pDataOffset+offsetof(XPE_DEF::IMAGE_IMPORT_DESCRIPTOR,Name));

                pIID->OriginalFirstThunk=pOIAT-pDataOffset;
                listPatches.append((char *)pIID-pDataOffset+offsetof(XPE_DEF::IMAGE_IMPORT_DESCRIPTOR,OriginalFirstThunk));

                strcpy(pAnsiData,pListHeaders->at(i).sName.toLatin1().data());
                pAnsiData+=pListHeaders->at(i).sName.length()+3;

                for(int j=0; j<pListHeaders->at(i).listPositions.count(); j++)
                {
                    if(pListHeaders->at(i).listPositions.at(j).sName!="")
                    {
                        *((quint32 *)pOIAT)=pAnsiData-pDataOffset;
                        *((quint32 *)pIAT)=*((quint32 *)pOIAT);

                        listPatches.append(pOIAT-pDataOffset);
                        listPatches.append(pIAT-pDataOffset);

                        *((quint16 *)pAnsiData)=pListHeaders->at(i).listPositions.at(j).nHint;
                        pAnsiData+=2;

                        strcpy(pAnsiData,pListHeaders->at(i).listPositions.at(j).sName.toLatin1().data());

                        pAnsiData+=pListHeaders->at(i).listPositions.at(j).sName.length()+1;
                    }
                    else
                    {
                        // TODO 64
                        if(nAddressSize==4)
                        {
                            *((quint32 *)pOIAT)=pListHeaders->at(i).listPositions.at(j).nOrdinal+0x80000000;
                            *((quint32 *)pIAT)=*((quint32 *)pOIAT);
                        }
                        else
                        {
                            *((quint64 *)pOIAT)=pListHeaders->at(i).listPositions.at(j).nOrdinal+0x8000000000000000;
                            *((quint64 *)pIAT)=*((quint64 *)pOIAT);
                        }
                    }

                    //            if(pListHeaders->at(i).nFirstThunk)
                    //            {
                    //                mapMove.insert(pListHeaders->at(i).listPositions.at(j).nThunkRVA,pIAT-pDataOffset);
                    //            }

                    pIAT+=nAddressSize;
                    pOIAT+=nAddressSize;
                }

                pIAT+=nAddressSize;
                pOIAT+=nAddressSize;
                pIID++;
            }

            XPE_DEF::IMAGE_SECTION_HEADER ish= {};

            ish.Characteristics=0xc0000040;

            // TODO section name!

            //    char *pszTest="ABCDE";

            //    addSection(sFileName,&ish,pszTest,5);

            if(addSection(pDevice,bIsImage,&ish,baImport.data(),baImport.size()))
            {
                QList<MEMORY_MAP> listMP=pe.getMemoryMapList();
                qint64 nBaseAddress=pe._getBaseAddress();

                XPE_DEF::IMAGE_DATA_DIRECTORY iddIAT= {};
                iddIAT.VirtualAddress=ish.VirtualAddress;
                iddIAT.Size=nIATSize;
                XPE_DEF::IMAGE_DATA_DIRECTORY iddImportTable= {};
                iddImportTable.VirtualAddress=nIATSize+ish.VirtualAddress;
                iddImportTable.Size=nImportTableSize;

                pe.setOptionalHeader_DataDirectory(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_IAT,&iddIAT);
                pe.setOptionalHeader_DataDirectory(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_IMPORT,&iddImportTable);

                for(int i=0; i<listPatches.count(); i++)
                {
                    // TODO 64
                    qint64 nCurrentOffset=ish.PointerToRawData+listPatches.at(i);
                    quint32 nValue=pe.read_uint32(nCurrentOffset);
                    pe.write_uint32(nCurrentOffset,nValue+ish.VirtualAddress);
                }

                for(int i=0; i<pListHeaders->count(); i++)
                {
                    if(pListHeaders->at(i).nFirstThunk)
                    {
                        XPE_DEF::IMAGE_IMPORT_DESCRIPTOR iid=pe.getImportDescriptor(i);

                        //                        qDebug("pListHeaders->at(i).nFirstThunk(%d): %x",i,(quint32)pListHeaders->at(i).nFirstThunk);
                        //                        qDebug("FirstThunk(%d): %x",i,(quint32)iid.FirstThunk);
                        //                        qDebug("Import offset(%d): %x",i,(quint32)pe.getDataDirectoryOffset(XPE_DEF::IMAGE_DIRECTORY_ENTRY_IMPORT));

                        qint64 nSrcOffset=pe.addressToOffset(&listMP,iid.FirstThunk+nBaseAddress);
                        qint64 nDstOffset=pe.addressToOffset(&listMP,pListHeaders->at(i).nFirstThunk+nBaseAddress);

                        //                        qDebug("src: %x",(quint32)nSrcOffset);
                        //                        qDebug("dst: %x",(quint32)nDstOffset);

                        if((nSrcOffset!=-1)&&(nDstOffset!=-1))
                        {
                            // TODO 64 ????
                            while(true)
                            {
                                quint32 nValue=pe.read_uint32(nSrcOffset);

                                pe.write_uint32(nDstOffset,nValue);

                                if(nValue==0)
                                {
                                    break;
                                }

                                nSrcOffset+=nAddressSize;
                                nDstOffset+=nAddressSize;
                            }

                            //                            iid.OriginalFirstThunk=0;
                            iid.FirstThunk=pListHeaders->at(i).nFirstThunk;

                            pe.setImportDescriptor(i,&iid);
                        }
                    }
                }

                bResult=true;
            }
        }
    }

    return bResult;
}

bool XPE::setImports(QString sFileName,bool bIsImage,QList<XPE::IMPORT_HEADER> *pListHeaders)
{
    bool bResult=false;

    QFile file(sFileName);

    if(file.open(QIODevice::ReadWrite))
    {
        bResult=setImports(&file,bIsImage,pListHeaders);

        file.close();
    }

    return bResult;
}

XPE::RESOURCE_HEADER XPE::getResourceHeader()
{
    RESOURCE_HEADER result= {};

    qint64 nResourceOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_RESOURCE);

    if(nResourceOffset!=-1)
    {
        QList<MEMORY_MAP> memoryMap=getMemoryMapList();
        qint64 nBaseAddress=_getBaseAddress();

        qint64 nOffset=nResourceOffset;

        result.nOffset=nOffset;
        result.directory=read_IMAGE_RESOURCE_DIRECTORY(nOffset);

        if((result.directory.NumberOfIdEntries+result.directory.NumberOfNamedEntries<=100)&&(result.directory.Characteristics==0)) // check corrupted
        {
            nOffset+=sizeof(XPE_DEF::IMAGE_RESOURCE_DIRECTORY);

            for(int i=0; i<result.directory.NumberOfIdEntries+result.directory.NumberOfNamedEntries; i++)
            {
                RESOURCE_POSITION rp=_getResourcePosition(&memoryMap,nBaseAddress,nResourceOffset,nOffset,0);

                if(!rp.bIsValid)
                {
                    break;
                }

                result.listPositions.append(rp);
                nOffset+=sizeof(XPE_DEF::IMAGE_RESOURCE_DIRECTORY_ENTRY);
            }
        }
    }

    return result;
}

QList<XPE::RESOURCE_RECORD> XPE::getResources()
{
    // TODO BE LE
    QList<RESOURCE_RECORD> listResources;

    qint64 nResourceOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_RESOURCE);

    if(nResourceOffset!=-1)
    {
        QList<MEMORY_MAP> memoryMap=getMemoryMapList();
        qint64 nBaseAddress=_getBaseAddress();
        RESOURCE_RECORD record= {};

        qint64 nOffsetLevel[3]= {};
        XPE_DEF::IMAGE_RESOURCE_DIRECTORY rd[3]= {};
        XPE_DEF::IMAGE_RESOURCE_DIRECTORY_ENTRY rde[3]= {};

#if (QT_VERSION_MAJOR>=5)&&(QT_VERSION_MINOR>=10)
        RESOURCES_ID_NAME irin[3]= {};
#else
        RESOURCES_ID_NAME irin[3]= {0}; // MinGW 4.9 bug?
#endif

        nOffsetLevel[0]=nResourceOffset;
        rd[0]=read_IMAGE_RESOURCE_DIRECTORY(nOffsetLevel[0]);

        if((rd[0].NumberOfIdEntries+rd[0].NumberOfNamedEntries<=100)&&(rd[0].Characteristics==0)) // check corrupted
        {
            nOffsetLevel[0]+=sizeof(XPE_DEF::IMAGE_RESOURCE_DIRECTORY);

            for(int i=0; i<rd[0].NumberOfIdEntries+rd[0].NumberOfNamedEntries; i++)
            {
                rde[0]=read_IMAGE_RESOURCE_DIRECTORY_ENTRY(nOffsetLevel[0]);

                irin[0]=getResourcesIDName(nResourceOffset,rde[0].Name);
                record.nID[0]=irin[0].nID;
                record.sName[0]=irin[0].sName;
                record.nNameOffset[0]=irin[0].nNameOffset;

                nOffsetLevel[1]=nResourceOffset+rde[0].OffsetToDirectory;

                rd[1]=read_IMAGE_RESOURCE_DIRECTORY(nOffsetLevel[1]);

                if(rd[1].Characteristics!=0)
                {
                    break;
                }

                nOffsetLevel[1]+=sizeof(XPE_DEF::IMAGE_RESOURCE_DIRECTORY);

                for(int j=0; j<rd[1].NumberOfIdEntries+rd[1].NumberOfNamedEntries; j++)
                {
                    rde[1]=read_IMAGE_RESOURCE_DIRECTORY_ENTRY(nOffsetLevel[1]);

                    irin[1]=getResourcesIDName(nResourceOffset,rde[1].Name);
                    record.nID[1]=irin[1].nID;
                    record.sName[1]=irin[1].sName;
                    record.nNameOffset[1]=irin[1].nNameOffset;

                    nOffsetLevel[2]=nResourceOffset+rde[1].OffsetToDirectory;

                    rd[2]=read_IMAGE_RESOURCE_DIRECTORY(nOffsetLevel[2]);

                    if(rd[2].Characteristics!=0)
                    {
                        break;
                    }

                    nOffsetLevel[2]+=sizeof(XPE_DEF::IMAGE_RESOURCE_DIRECTORY);

                    for(int k=0; k<rd[2].NumberOfIdEntries+rd[2].NumberOfNamedEntries; k++)
                    {
                        rde[2]=read_IMAGE_RESOURCE_DIRECTORY_ENTRY(nOffsetLevel[2]);

                        irin[2]=getResourcesIDName(nResourceOffset,rde[2].Name);
                        record.nID[2]=irin[2].nID;
                        record.sName[2]=irin[2].sName;
                        record.nNameOffset[2]=irin[2].nNameOffset;

                        record.nIRDEOffset=rde[2].OffsetToData;
                        XPE_DEF::IMAGE_RESOURCE_DATA_ENTRY irde=read_IMAGE_RESOURCE_DATA_ENTRY(nResourceOffset+record.nIRDEOffset);
                        record.nRVA=irde.OffsetToData;
                        record.nAddress=irde.OffsetToData+nBaseAddress;
                        record.nOffset=addressToOffset(&memoryMap,record.nAddress);
                        record.nSize=irde.Size;

                        listResources.append(record);

                        nOffsetLevel[2]+=sizeof(XPE_DEF::IMAGE_RESOURCE_DIRECTORY_ENTRY);
                    }

                    nOffsetLevel[1]+=sizeof(XPE_DEF::IMAGE_RESOURCE_DIRECTORY_ENTRY);
                }

                nOffsetLevel[0]+=sizeof(XPE_DEF::IMAGE_RESOURCE_DIRECTORY_ENTRY);
            }
        }
    }

    return listResources;
}

XPE::RESOURCE_RECORD XPE::getResourceRecord(quint32 nID1, quint32 nID2, QList<XPE::RESOURCE_RECORD> *pListRecords)
{
    RESOURCE_RECORD result= {};
    result.nOffset=-1;

    for(int i=0; i<pListRecords->count(); i++)
    {
        if(pListRecords->at(i).nID[0]==nID1)
        {
            if((pListRecords->at(i).nID[1]==nID2)||(nID2==(quint32)-1))
            {
                result=pListRecords->at(i);

                break;
            }
        }
    }

    return result;
}

XPE::RESOURCE_RECORD XPE::getResourceRecord(quint32 nID1, QString sName2, QList<XPE::RESOURCE_RECORD> *pListRecords)
{
    RESOURCE_RECORD result= {};
    result.nOffset=-1;

    for(int i=0; i<pListRecords->count(); i++)
    {
        if((pListRecords->at(i).nID[0]==nID1)&&(pListRecords->at(i).sName[1]==sName2))
        {
            result=pListRecords->at(i);

            break;
        }
    }

    return result;
}

XPE::RESOURCE_RECORD XPE::getResourceRecord(QString sName1, quint32 nID2, QList<XPE::RESOURCE_RECORD> *pListRecords)
{
    RESOURCE_RECORD result= {};
    result.nOffset=-1;

    for(int i=0; i<pListRecords->count(); i++)
    {
        if(pListRecords->at(i).sName[0]==sName1)
        {
            if((pListRecords->at(i).nID[1]==nID2)||(nID2==-1))
            {
                result=pListRecords->at(i);

                break;
            }
        }
    }

    return result;
}

XPE::RESOURCE_RECORD XPE::getResourceRecord(QString sName1, QString sName2, QList<XPE::RESOURCE_RECORD> *pListRecords)
{
    RESOURCE_RECORD result= {};
    result.nOffset=-1;

    for(int i=0; i<pListRecords->count(); i++)
    {
        if((pListRecords->at(i).sName[0]==sName1)&&(pListRecords->at(i).sName[1]==sName2))
        {
            result=pListRecords->at(i);

            break;
        }
    }

    return result;
}

bool XPE::isResourcePresent(quint32 nID1, quint32 nID2, QList<XPE::RESOURCE_RECORD> *pListRecords)
{
    return (getResourceRecord(nID1,nID2,pListRecords).nSize);
}

bool XPE::isResourcePresent(quint32 nID1, QString sName2, QList<XPE::RESOURCE_RECORD> *pListRecords)
{
    return (getResourceRecord(nID1,sName2,pListRecords).nSize);
}

bool XPE::isResourcePresent(QString sName1, quint32 nID2, QList<XPE::RESOURCE_RECORD> *pListRecords)
{
    return (getResourceRecord(sName1,nID2,pListRecords).nSize);
}

bool XPE::isResourcePresent(QString sName1, QString sName2, QList<XPE::RESOURCE_RECORD> *pListRecords)
{
    return (getResourceRecord(sName1,sName2,pListRecords).nSize);
}

QString XPE::getResourceManifest(QList<XPE::RESOURCE_RECORD> *pListRecords)
{
    QString sResult;

    RESOURCE_RECORD rh=getResourceRecord(XPE_DEF::S_RT_MANIFEST,1,pListRecords);

    if(rh.nOffset!=-1)
    {
        rh.nSize=qMin(rh.nSize,qint64(4000));
        sResult=read_ansiString(rh.nOffset,rh.nSize);
    }

    return sResult;
}

XPE_DEF::S_VS_VERSION_INFO XPE::readResourceVersionInfo(qint64 nOffset)
{
    XPE_DEF::S_VS_VERSION_INFO result= {};

    read_array(nOffset,(char *)&result,sizeof(XPE_DEF::S_VS_VERSION_INFO));

    return result;
}

quint32 XPE::__getResourceVersion(XPE::RESOURCE_VERSION *pResult, qint64 nOffset, qint64 nSize, QString sPrefix, int nLevel)
{
    quint32 nResult=0;

    if((quint32)nSize>=sizeof(XPE_DEF::S_VS_VERSION_INFO))
    {
        XPE_DEF::S_VS_VERSION_INFO vi=readResourceVersionInfo(nOffset);

        if(vi.wLength<=nSize)
        {
            if(vi.wValueLength<vi.wLength)
            {
                QString sTitle=read_unicodeString(nOffset+sizeof(XPE_DEF::S_VS_VERSION_INFO));

                qint32 nDelta=sizeof(XPE_DEF::S_VS_VERSION_INFO);
                nDelta+=(sTitle.length()+1)*sizeof(quint16);
                nDelta=S_ALIGN_UP(nDelta,4);

                if(sPrefix!="")
                {
                    sPrefix+=".";
                }

                sPrefix+=sTitle;

                if(sPrefix=="VS_VERSION_INFO")
                {
                    if(vi.wValueLength>=sizeof(XPE_DEF::S_tagVS_FIXEDFILEINFO))
                    {
                        // TODO Check Signature?
                        pResult->fileInfo.dwSignature=read_uint32(nOffset+nDelta+offsetof(XPE_DEF::S_tagVS_FIXEDFILEINFO,dwSignature));
                        pResult->fileInfo.dwStrucVersion=read_uint32(nOffset+nDelta+offsetof(XPE_DEF::S_tagVS_FIXEDFILEINFO,dwStrucVersion));
                        pResult->fileInfo.dwFileVersionMS=read_uint32(nOffset+nDelta+offsetof(XPE_DEF::S_tagVS_FIXEDFILEINFO,dwFileVersionMS));
                        pResult->fileInfo.dwFileVersionLS=read_uint32(nOffset+nDelta+offsetof(XPE_DEF::S_tagVS_FIXEDFILEINFO,dwFileVersionLS));
                        pResult->fileInfo.dwProductVersionMS=read_uint32(nOffset+nDelta+offsetof(XPE_DEF::S_tagVS_FIXEDFILEINFO,dwProductVersionMS));
                        pResult->fileInfo.dwProductVersionLS=read_uint32(nOffset+nDelta+offsetof(XPE_DEF::S_tagVS_FIXEDFILEINFO,dwProductVersionLS));
                        pResult->fileInfo.dwFileFlagsMask=read_uint32(nOffset+nDelta+offsetof(XPE_DEF::S_tagVS_FIXEDFILEINFO,dwFileFlagsMask));
                        pResult->fileInfo.dwFileFlags=read_uint32(nOffset+nDelta+offsetof(XPE_DEF::S_tagVS_FIXEDFILEINFO,dwFileFlags));
                        pResult->fileInfo.dwFileOS=read_uint32(nOffset+nDelta+offsetof(XPE_DEF::S_tagVS_FIXEDFILEINFO,dwFileOS));
                        pResult->fileInfo.dwFileType=read_uint32(nOffset+nDelta+offsetof(XPE_DEF::S_tagVS_FIXEDFILEINFO,dwFileType));
                        pResult->fileInfo.dwFileSubtype=read_uint32(nOffset+nDelta+offsetof(XPE_DEF::S_tagVS_FIXEDFILEINFO,dwFileSubtype));
                        pResult->fileInfo.dwFileDateMS=read_uint32(nOffset+nDelta+offsetof(XPE_DEF::S_tagVS_FIXEDFILEINFO,dwFileDateMS));
                        pResult->fileInfo.dwFileDateLS=read_uint32(nOffset+nDelta+offsetof(XPE_DEF::S_tagVS_FIXEDFILEINFO,dwFileDateLS));
                    }
                }

                if(nLevel==3)
                {
                    QString sValue=read_unicodeString(nOffset+nDelta);
                    sPrefix+=QString(":%1").arg(sValue);

                    pResult->listRecords.append(sPrefix);
                }

                if(sPrefix=="VS_VERSION_INFO.VarFileInfo.Translation")
                {
                    if(vi.wValueLength==4)
                    {
                        quint32 nValue=read_uint32(nOffset+nDelta);
                        QString sValue=XBinary::valueToHex(nValue);
                        sPrefix+=QString(":%1").arg(sValue);

                        pResult->listRecords.append(sPrefix);
                    }
                }

                nDelta+=vi.wValueLength;

                qint32 _nSize=vi.wLength-nDelta;

                if(nLevel<3)
                {
                    while(_nSize>0)
                    {
                        qint32 _nDelta=__getResourceVersion(pResult,nOffset+nDelta,vi.wLength-nDelta,sPrefix,nLevel+1);

                        if(_nDelta==0)
                        {
                            break;
                        }

                        _nDelta=S_ALIGN_UP(_nDelta,4);

                        nDelta+=_nDelta;
                        _nSize-=_nDelta;
                    }
                }

                nResult=vi.wLength;
            }
        }
    }

    return nResult;
}

XPE::RESOURCE_VERSION XPE::getResourceVersion(QList<XPE::RESOURCE_RECORD> *pListHeaders)
{
    RESOURCE_VERSION result= {};

    RESOURCE_RECORD rh=getResourceRecord(XPE_DEF::S_RT_VERSION,1,pListHeaders);

    if(rh.nOffset!=-1)
    {
        __getResourceVersion(&result,rh.nOffset,rh.nSize,"",0);
    }

    return result;
}

QString XPE::getResourceVersionValue(QString sKey,XPE::RESOURCE_VERSION *pResVersion)
{
    QString sResult;

    for(int i=0; i<pResVersion->listRecords.count(); i++)
    {
        QString sRecord=pResVersion->listRecords.at(i).section(".",3,-1);
        QString _sKey=sRecord.section(":",0,0);

        if(_sKey==sKey)
        {
            sResult=sRecord.section(":",1,-1);

            break;
        }
    }

    return sResult;
}

XPE_DEF::IMAGE_IMPORT_DESCRIPTOR XPE::read_IMAGE_IMPORT_DESCRIPTOR(qint64 nOffset)
{
    XPE_DEF::IMAGE_IMPORT_DESCRIPTOR result= {};

    read_array(nOffset,(char *)&result,sizeof(XPE_DEF::IMAGE_IMPORT_DESCRIPTOR));

    return result;
}

void XPE::write_IMAGE_IMPORT_DESCRIPTOR(qint64 nOffset, XPE_DEF::IMAGE_IMPORT_DESCRIPTOR value)
{
    write_array(nOffset,(char *)&value,sizeof(XPE_DEF::IMAGE_IMPORT_DESCRIPTOR));
}

bool XPE::isExportPresent()
{
    return isOptionalHeader_DataDirectoryPresent(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_EXPORT);
}

XPE::EXPORT_HEADER XPE::getExport()
{
    EXPORT_HEADER result= {};

    qint64 nExportOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_EXPORT);

    if(nExportOffset!=-1)
    {
        read_array(nExportOffset,(char *)&result.directory,sizeof(XPE_DEF::IMAGE_EXPORT_DIRECTORY));

        QList<MEMORY_MAP> listMemoryMap=getMemoryMapList();
        qint64 nBaseAddress=_getBaseAddress();

        qint64 nNameOffset=addressToOffset(&listMemoryMap,result.directory.Name+nBaseAddress);

        if(nNameOffset!=-1)
        {
            result.sName=read_ansiString(nNameOffset);
        }

        qint64 nAddressOfFunctionsOffset=addressToOffset(&listMemoryMap,result.directory.AddressOfFunctions+nBaseAddress);
        qint64 nAddressOfNamesOffset=addressToOffset(&listMemoryMap,result.directory.AddressOfNames+nBaseAddress);
        qint64 nAddressOfNameOrdinalsOffset=addressToOffset(&listMemoryMap,result.directory.AddressOfNameOrdinals+nBaseAddress);

        if(result.directory.NumberOfFunctions<0xFFFF)
        {
            if((nAddressOfFunctionsOffset!=-1)&&(nAddressOfNamesOffset!=-1)&&(nAddressOfNameOrdinalsOffset!=-1))
            {
                for(int i=0; i<(int)result.directory.NumberOfFunctions; i++)
                {
                    EXPORT_POSITION position= {};

                    int nIndex=read_uint16(nAddressOfNameOrdinalsOffset+2*i);
                    position.nOrdinal=nIndex+result.directory.Base;
                    position.nRVA=read_uint32(nAddressOfFunctionsOffset+4*nIndex);
                    position.nAddress=position.nRVA+nBaseAddress;
                    position.nNameRVA=read_uint32(nAddressOfNamesOffset+4*i);

                    qint64 nFunctionNameOffset=addressToOffset(&listMemoryMap,position.nNameRVA+nBaseAddress);

                    if(nFunctionNameOffset!=-1)
                    {
                        position.sFunctionName=read_ansiString(nFunctionNameOffset);
                    }

                    result.listPositions.append(position);
                }
            }
        }
    }

    return result;
}

bool XPE::isExportFunctionPresent(QString sFunction,XPE::EXPORT_HEADER *pExportHeader)
{
    bool bResult=false;

    for(int i=0; i<pExportHeader->listPositions.count(); i++)
    {
        if(pExportHeader->listPositions.at(i).sFunctionName==sFunction)
        {
            bResult=true;

            break;
        }
    }

    return bResult;
}

XPE_DEF::IMAGE_EXPORT_DIRECTORY XPE::getExportDirectory()
{
    XPE_DEF::IMAGE_EXPORT_DIRECTORY result= {};

    qint64 nExportOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_EXPORT);

    if(nExportOffset!=-1)
    {
        read_array(nExportOffset,(char *)&result,sizeof(XPE_DEF::IMAGE_EXPORT_DIRECTORY));
    }

    return result;
}

void XPE::setExportDirectory(XPE_DEF::IMAGE_EXPORT_DIRECTORY *pExportDirectory)
{
    qint64 nExportOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_EXPORT);

    if(nExportOffset!=-1)
    {
        write_array(nExportOffset,(char *)pExportDirectory,sizeof(XPE_DEF::IMAGE_EXPORT_DIRECTORY));
    }
}

void XPE::setExportDirectory_Characteristics(quint32 value)
{
    qint64 nExportOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_EXPORT);

    if(nExportOffset!=-1)
    {
        write_uint32(nExportOffset+offsetof(XPE_DEF::IMAGE_EXPORT_DIRECTORY,Characteristics),value);
    }
}

void XPE::setExportDirectory_TimeDateStamp(quint32 value)
{
    qint64 nExportOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_EXPORT);

    if(nExportOffset!=-1)
    {
        write_uint32(nExportOffset+offsetof(XPE_DEF::IMAGE_EXPORT_DIRECTORY,TimeDateStamp),value);
    }
}

void XPE::setExportDirectory_MajorVersion(quint16 value)
{
    qint64 nExportOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_EXPORT);

    if(nExportOffset!=-1)
    {
        write_uint16(nExportOffset+offsetof(XPE_DEF::IMAGE_EXPORT_DIRECTORY,MajorVersion),value);
    }
}

void XPE::setExportDirectory_MinorVersion(quint16 value)
{
    qint64 nExportOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_EXPORT);

    if(nExportOffset!=-1)
    {
        write_uint16(nExportOffset+offsetof(XPE_DEF::IMAGE_EXPORT_DIRECTORY,MinorVersion),value);
    }
}

void XPE::setExportDirectory_Name(quint32 value)
{
    qint64 nExportOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_EXPORT);

    if(nExportOffset!=-1)
    {
        write_uint32(nExportOffset+offsetof(XPE_DEF::IMAGE_EXPORT_DIRECTORY,Name),value);
    }
}

void XPE::setExportDirectory_Base(quint32 value)
{
    qint64 nExportOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_EXPORT);

    if(nExportOffset!=-1)
    {
        write_uint32(nExportOffset+offsetof(XPE_DEF::IMAGE_EXPORT_DIRECTORY,Base),value);
    }
}

void XPE::setExportDirectory_NumberOfFunctions(quint32 value)
{
    qint64 nExportOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_EXPORT);

    if(nExportOffset!=-1)
    {
        write_uint32(nExportOffset+offsetof(XPE_DEF::IMAGE_EXPORT_DIRECTORY,NumberOfFunctions),value);
    }
}

void XPE::setExportDirectory_NumberOfNames(quint32 value)
{
    qint64 nExportOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_EXPORT);

    if(nExportOffset!=-1)
    {
        write_uint32(nExportOffset+offsetof(XPE_DEF::IMAGE_EXPORT_DIRECTORY,NumberOfNames),value);
    }
}

void XPE::setExportDirectory_AddressOfFunctions(quint32 value)
{
    qint64 nExportOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_EXPORT);

    if(nExportOffset!=-1)
    {
        write_uint32(nExportOffset+offsetof(XPE_DEF::IMAGE_EXPORT_DIRECTORY,AddressOfFunctions),value);
    }
}

void XPE::setExportDirectory_AddressOfNames(quint32 value)
{
    qint64 nExportOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_EXPORT);

    if(nExportOffset!=-1)
    {
        write_uint32(nExportOffset+offsetof(XPE_DEF::IMAGE_EXPORT_DIRECTORY,AddressOfNames),value);
    }
}

void XPE::setExportDirectory_AddressOfNameOrdinals(quint32 value)
{
    qint64 nExportOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_EXPORT);

    if(nExportOffset!=-1)
    {
        write_uint32(nExportOffset+offsetof(XPE_DEF::IMAGE_EXPORT_DIRECTORY,AddressOfNameOrdinals),value);
    }
}

QByteArray XPE::getHeaders()
{
    QByteArray baResult;

    int nSizeOfHeaders=getOptionalHeader_SizeOfHeaders();

    if(isImage())
    {
        quint32 nSectionAlignment=getOptionalHeader_SectionAlignment();
        nSizeOfHeaders=S_ALIGN_UP(nSizeOfHeaders,nSectionAlignment);
    }

    baResult=read_array(0,nSizeOfHeaders);

    if(baResult.size()!=nSizeOfHeaders)
    {
        baResult.resize(0);
    }

    return baResult;
}

XBinary::OFFSETSIZE XPE::__getSectionOffsetAndSize(quint32 nSection)
{
    OFFSETSIZE result= {};

    XPE_DEF::IMAGE_SECTION_HEADER sectionHeader=getSectionHeader(nSection);
    quint32 nSectionAlignment=getOptionalHeader_SectionAlignment();
    quint32 nFileAlignment=getOptionalHeader_FileAlignment();

    if(nFileAlignment==nSectionAlignment)
    {
        nFileAlignment=1;
    }

    bool bIsSectionValid=false;

    if(!isImage())
    {
        bIsSectionValid=(bool)(sectionHeader.SizeOfRawData!=0);
    }
    else
    {
        bIsSectionValid=(bool)(sectionHeader.Misc.VirtualSize!=0);
    }

    if(bIsSectionValid)
    {
        qint64 nSectionOffset=0;
        qint64 nSectionSize=0;

        if(!isImage())
        {
            nSectionOffset=sectionHeader.PointerToRawData;
            nSectionOffset=S_ALIGN_DOWN(nSectionOffset,nFileAlignment);
            nSectionSize=sectionHeader.SizeOfRawData+(sectionHeader.PointerToRawData-nSectionOffset);
        }
        else
        {
            nSectionOffset=sectionHeader.VirtualAddress;
            nSectionSize=sectionHeader.Misc.VirtualSize;
        }

        result=convertOffsetAndSize(nSectionOffset,nSectionSize);
    }
    else
    {
        result.nOffset=-1;
    }

    return result;
}

QByteArray XPE::getSection(quint32 nSection)
{
    QByteArray baResult;
    OFFSETSIZE offsize=__getSectionOffsetAndSize(nSection);

    if(offsize.nOffset!=-1)
    {
        baResult=read_array(offsize.nOffset,offsize.nSize);

        if(baResult.size()!=offsize.nSize) // TODO check???
        {
            baResult.resize(0);
        }
    }

    return baResult;
}

QString XPE::getSectionMD5(quint32 nSection)
{
    QString sResult;
    OFFSETSIZE offsize=__getSectionOffsetAndSize(nSection);

    if(offsize.nOffset!=-1)
    {
        sResult=getMD5(offsize.nOffset,offsize.nSize);
    }

    return sResult;
}

double XPE::getSectionEntropy(quint32 nSection)
{
    double dResult=0;
    OFFSETSIZE offsize=__getSectionOffsetAndSize(nSection);

    if(offsize.nOffset!=-1)
    {
        dResult=getEntropy(offsize.nOffset,offsize.nSize);
    }

    return dResult;
}

qint32 XPE::addressToSection(qint64 nAddress)
{
    QList<MEMORY_MAP> list=getMemoryMapList();

    return addressToSection(&list,nAddress);
}

qint32 XPE::addressToSection(QList<XBinary::MEMORY_MAP> *pMemoryMap, qint64 nAddress)
{
    qint32 nResult=-1;

    MEMORY_MAP mm=getAddressMemoryMap(pMemoryMap,nAddress);

    if(mm.bIsLoadSection)
    {
        nResult=mm.nLoadSection;
    }

    return nResult;
}

bool XPE::addImportSection(QMap<qint64, QString> *pMapIAT)
{
    return addImportSection(getDevice(),isImage(),pMapIAT);
}

bool XPE::addImportSection(QIODevice *pDevice, bool bIsImage, QMap<qint64, QString> *pMapIAT)
{
#ifdef QT_DEBUG
    QElapsedTimer timer;
    timer.start();
    qDebug("QPE::addImportSection");
#endif

    bool bResult=false;

    if(isResizeEnable(pDevice))
    {
        XPE pe(pDevice,bIsImage);

        if(pe.isValid())
        {
            QList<XPE::IMPORT_HEADER> list=mapIATToList(pMapIAT,pe.is64());
#ifdef QT_DEBUG
            qDebug("QPE::addImportSection:mapIATToList: %lld msec",timer.elapsed());
#endif
            bResult=setImports(pDevice,bIsImage,&list);
#ifdef QT_DEBUG
            qDebug("QPE::addImportSection:setImports: %lld msec",timer.elapsed());
#endif
        }
    }

#ifdef QT_DEBUG
    qDebug("QPE::addImportSection: %lld msec",timer.elapsed());
#endif

    return bResult;
}

bool XPE::addImportSection(QString sFileName,bool bIsImage, QMap<qint64, QString> *pMapIAT)
{
    bool bResult=false;
    QFile file(sFileName);

    if(file.open(QIODevice::ReadWrite))
    {
        bResult=addImportSection(&file,bIsImage,pMapIAT);

        file.close();
    }

    return bResult;
}

QList<XPE::IMPORT_HEADER> XPE::mapIATToList(QMap<qint64, QString> *pMapIAT,bool bIs64)
{
    QList<XPE::IMPORT_HEADER> listResult;

    IMPORT_HEADER record= {};

    quint64 nCurrentRVA=0;

    quint32 nStep=0;

    if(bIs64)
    {
        nStep=8;
    }
    else
    {
        nStep=4;
    }

    QMapIterator<qint64, QString> i(*pMapIAT);

    while(i.hasNext())
    {
        i.next();

        QString sLibrary=i.value().section("#",0,0);
        QString sFunction=i.value().section("#",1,1);

        if(((qint64)(nCurrentRVA+nStep)!=i.key())||((record.sName!="")&&(record.sName!=sLibrary)))
        {
            if(record.sName!="")
            {
                listResult.append(record);
            }

            record.sName=sLibrary;
            record.nFirstThunk=i.key();
            record.listPositions.clear();
        }

        nCurrentRVA=i.key();

        IMPORT_POSITION position= {};

        position.nHint=0;

        if(sFunction.toInt())
        {
            position.nOrdinal=sFunction.toInt();
        }
        else
        {
            position.sName=sFunction;
        }

        position.nThunkRVA=i.key();

        record.listPositions.append(position);

        if(!i.hasNext())
        {
            if(record.sName!="")
            {
                listResult.append(record);
            }
        }
    }

    return listResult;
}

quint32 XPE::calculateCheckSum()
{
    quint32 nCalcSum=_checkSum(0,getSize());
    quint32 nHdrSum=getOptionalHeader_CheckSum();

    if(S_LOWORD(nCalcSum)>=S_LOWORD(nHdrSum))
    {
        nCalcSum-=S_LOWORD(nHdrSum);
    }
    else
    {
        nCalcSum=((S_LOWORD(nCalcSum)-S_LOWORD(nHdrSum))&0xFFFF)-1;
    }

    if(S_LOWORD(nCalcSum)>=S_HIWORD(nHdrSum)) //!!!!!
    {
        nCalcSum-=S_HIWORD(nHdrSum);
    }
    else
    {
        nCalcSum=((S_LOWORD(nCalcSum)-S_HIWORD(nHdrSum))&0xFFFF)-1;
    }

    nCalcSum+=getSize();

    return nCalcSum;
}

qint64 XPE::getOverlaySize()
{
    qint64 nSize=getSize();
    qint64 nRawSize=_calculateRawSize();
    qint64 nDelta=nSize-nRawSize;

    return qMax(nDelta,(qint64)0);
}

qint64 XPE::getOverlayOffset()
{
    return _calculateRawSize();
}

bool XPE::isOverlayPresent()
{
    return getOverlaySize()!=0;
}

bool XPE::addOverlay(char *pData, qint64 nDataSize)
{
    return addOverlay(getDevice(),isImage(),pData,nDataSize);
}

bool XPE::addOverlay(QString sFileName,bool bIsImage, char *pData, qint64 nDataSize)
{
    bool bResult=false;
    QFile file;
    file.setFileName(sFileName);

    if(file.open(QIODevice::ReadWrite))
    {
        bResult=addOverlay(&file,bIsImage,pData,nDataSize);

        file.close();
    }

    return bResult;
}

bool XPE::addOverlay(QIODevice *pDevice,bool bIsImage, char *pData, qint64 nDataSize)
{
    bool bResult=false;

    if(isResizeEnable(pDevice))
    {
        XPE pe(pDevice,bIsImage);

        if(pe.isValid())
        {
            qint64 nRawSize=pe.getOverlayOffset();

            resize(pDevice,nRawSize+nDataSize);

            if(nDataSize)
            {
                pe.write_array(nRawSize,pData,nDataSize);
            }

            pe._fixCheckSum();
            bResult=true;
        }
    }

    return bResult;
}

bool XPE::addOverlayFromDevice(QIODevice *pSourceDevice, qint64 nOffset, qint64 nSize)
{
    return addOverlayFromDevice(getDevice(),isImage(),pSourceDevice,nOffset,nSize);
}

bool XPE::addOverlayFromDevice(QIODevice *pDevice, bool bIsImage, QIODevice *pSourceDevice, qint64 nOffset, qint64 nSize)
{
    bool bResult=false;
    const int BUFFER_SIZE=0x1000;

    if(isResizeEnable(pDevice))
    {
        XPE pe(pDevice,bIsImage);

        if(pe.isValid())
        {
            qint64 nRawSize=pe.getOverlayOffset();
            resize(pDevice,nRawSize+nSize);

            if(nSize)
            {
                char *pBuffer=new char[BUFFER_SIZE];

                qint64 nSourceOffset=nOffset;
                qint64 nDestOffset=pe.getOverlayOffset();

                bResult=true;

                while(nSize>0)
                {
                    qint64 nTemp=qMin((qint64)(BUFFER_SIZE),nSize);

                    pSourceDevice->seek(nOffset);

                    if(pSourceDevice->read(pBuffer,nTemp)<=0)
                    {
                        bResult=false;

                        break;
                    }

                    write_array(nDestOffset,pBuffer,nTemp);

                    nSourceOffset+=nTemp;
                    nDestOffset+=nTemp;
                    nSize-=nTemp;
                }

                delete [] pBuffer;
            }

            pe._fixCheckSum();
        }
    }

    return bResult;
}

bool XPE::removeOverlay()
{
    return removeOverlay(getDevice(),isImage());
}

bool XPE::removeOverlay(QIODevice *pDevice,bool bIsImage)
{
    bool bResult=false;

    if(isResizeEnable(pDevice))
    {
        XPE pe(pDevice,bIsImage);

        if(pe.isValid())
        {
            bResult=addOverlay(pDevice,bIsImage,0,0);
        }
    }

    return bResult;
}

bool XPE::addSection(QString sFileName,bool bIsImage, XPE_DEF::IMAGE_SECTION_HEADER *pSectionHeader, char *pData, qint64 nDataSize)
{
    bool bResult=false;
    QFile file(sFileName);

    if(file.open(QIODevice::ReadWrite))
    {
        bResult=addSection(&file,bIsImage,pSectionHeader,pData,nDataSize);

        file.close();
    }
    else
    {
        qDebug("Cannot open file"); // TODO emit
    }

    return bResult;
}

bool XPE::addSection(QIODevice *pDevice, bool bIsImage, XPE_DEF::IMAGE_SECTION_HEADER *pSectionHeader, char *pData, qint64 nDataSize)
{
    bool bResult=false;

    if(isResizeEnable(pDevice))
    {
        XPE pe(pDevice,bIsImage);

        if(pe.isValid())
        {
            qint64 nHeadersSize=pe._fixHeadersSize();
            qint64 nNewHeadersSize=pe._calculateHeadersSize(pe.getSectionsTableOffset(),pe.getFileHeader_NumberOfSections()+1);
            quint32 nFileAlignment=pe.getOptionalHeader_FileAlignment();
            quint32 nSectionAlignment=pe.getOptionalHeader_SectionAlignment();

            if(pSectionHeader->PointerToRawData==0)
            {
                pSectionHeader->PointerToRawData=pe._calculateRawSize();
            }

            if(pSectionHeader->SizeOfRawData==0)
            {
                pSectionHeader->SizeOfRawData=S_ALIGN_UP(nDataSize,nFileAlignment);
            }

            if(pSectionHeader->VirtualAddress==0)
            {
                pSectionHeader->VirtualAddress=S_ALIGN_UP(pe.getOptionalHeader_SizeOfImage(),nSectionAlignment);
            }

            if(pSectionHeader->Misc.VirtualSize==0)
            {
                pSectionHeader->Misc.VirtualSize=S_ALIGN_UP(nDataSize,nSectionAlignment);
            }

            qint64 nDelta=nNewHeadersSize-nHeadersSize;
            qint64 nFileSize=pDevice->size();

            if(nDelta>0)
            {
                resize(pDevice,nFileSize+nDelta);
                pe.moveMemory(nHeadersSize,nNewHeadersSize,nFileSize-nHeadersSize);
            }
            else if(nDelta<0)
            {
                pe.moveMemory(nHeadersSize,nNewHeadersSize,nFileSize-nHeadersSize);
                resize(pDevice,nFileSize+nDelta);
            }

            pe._fixFileOffsets(nDelta);

            pSectionHeader->PointerToRawData+=nDelta;
            nFileSize+=nDelta;

            // TODO!!!
            resize(pDevice,nFileSize+pSectionHeader->SizeOfRawData);

            quint32 nNumberOfSections=pe.getFileHeader_NumberOfSections();
            pe.setFileHeader_NumberOfSections(nNumberOfSections+1);
            pe.setSectionHeader(nNumberOfSections,pSectionHeader);

            // Overlay
            if(pe.isOverlayPresent())
            {
                qint64 nOverlayOffset=pe.getOverlayOffset();
                qint64 nOverlaySize=pe.getOverlaySize();
                pe.moveMemory(nOverlayOffset-pSectionHeader->SizeOfRawData,nOverlayOffset,nOverlaySize);
            }

            pe.write_array(pSectionHeader->PointerToRawData,pData,nDataSize);

            pe.zeroFill(pSectionHeader->PointerToRawData+nDataSize,(pSectionHeader->SizeOfRawData)-nDataSize);

            qint64 nNewImageSize=S_ALIGN_UP(pSectionHeader->VirtualAddress+pSectionHeader->Misc.VirtualSize,nSectionAlignment);
            pe.setOptionalHeader_SizeOfImage(nNewImageSize);

            // TODO flag
            pe._fixCheckSum();

            bResult=true;
        }
    }

    return bResult;
}

bool XPE::removeLastSection()
{
    return removeLastSection(getDevice(),isImage());
}

bool XPE::removeLastSection(QIODevice *pDevice,bool bIsImage)
{
    bool bResult=false;

    if(isResizeEnable(pDevice))
    {
        XPE pe(pDevice,bIsImage);

        if(pe.isValid())
        {
            int nNumberOfSections=pe.getFileHeader_NumberOfSections();

            if(nNumberOfSections)
            {
                qint64 nHeadersSize=pe._fixHeadersSize();
                qint64 nNewHeadersSize=pe._calculateHeadersSize(pe.getSectionsTableOffset(),nNumberOfSections-1);
                quint32 nFileAlignment=pe.getOptionalHeader_FileAlignment();
                quint32 nSectionAlignment=pe.getOptionalHeader_SectionAlignment();
                bool bIsOverlayPresent=pe.isOverlayPresent();
                qint64 nOverlayOffset=pe.getOverlayOffset();
                qint64 nOverlaySize=pe.getOverlaySize();

                XPE_DEF::IMAGE_SECTION_HEADER ish=pe.getSectionHeader(nNumberOfSections-1);
                XPE_DEF::IMAGE_SECTION_HEADER ish0= {};
                pe.setSectionHeader(nNumberOfSections-1,&ish0);
                pe.setFileHeader_NumberOfSections(nNumberOfSections-1);

                ish.SizeOfRawData=S_ALIGN_UP(ish.SizeOfRawData,nFileAlignment);
                ish.Misc.VirtualSize=S_ALIGN_UP(ish.Misc.VirtualSize,nSectionAlignment);

                qint64 nDelta=nNewHeadersSize-nHeadersSize;
                qint64 nFileSize=pDevice->size();

                if(nDelta>0)
                {
                    resize(pDevice,nFileSize+nDelta);
                    pe.moveMemory(nHeadersSize,nNewHeadersSize,nFileSize-nHeadersSize);
                }
                else if(nDelta<0)
                {
                    pe.moveMemory(nHeadersSize,nNewHeadersSize,nFileSize-nHeadersSize);
                    resize(pDevice,nFileSize+nDelta);
                }

                pe._fixFileOffsets(nDelta);

                nFileSize+=nDelta;
                nOverlayOffset+=nDelta;

                if(bIsOverlayPresent)
                {
                    pe.moveMemory(nOverlayOffset,nOverlayOffset-ish.SizeOfRawData,nOverlaySize);
                }

                resize(pDevice,nFileSize-ish.SizeOfRawData);

                qint64 nNewImageSize=S_ALIGN_UP(ish.VirtualAddress,nSectionAlignment);
                pe.setOptionalHeader_SizeOfImage(nNewImageSize);

                pe._fixCheckSum();

                bResult=true;
            }
        }
    }

    return bResult;
}

bool XPE::removeLastSection(QString sFileName, bool bIsImage)
{
    bool bResult=false;
    QFile file(sFileName);

    if(file.open(QIODevice::ReadWrite))
    {
        bResult=removeLastSection(&file,bIsImage);

        file.close();
    }

    return bResult;
}

bool XPE::removeOverlay(QString sFileName,bool bIsImage)
{
    return addOverlay(sFileName,bIsImage,0,0);
}

bool XPE::addSection(XPE_DEF::IMAGE_SECTION_HEADER *pSectionHeader, char *pData, qint64 nDataSize)
{
    return addSection(getDevice(),isImage(),pSectionHeader,pData,nDataSize);
}

qint64 XPE::_calculateRawSize()
{
    qint64 nResult=0;
    QList<MEMORY_MAP> list=getMemoryMapList();

    for(int i=0; i<list.count(); i++)
    {
        if(list.at(i).bIsOvelay)
        {
            nResult=list.at(i).nOffset;

            break;
        }
    }

    return nResult;
}

XPE::RESOURCE_POSITION XPE::_getResourcePosition(QList<XBinary::MEMORY_MAP> *pMemoryMap,qint64 nBaseAddress, qint64 nResourceOffset, qint64 nOffset, quint32 nLevel)
{
    RESOURCE_POSITION result= {};

    result.nOffset=nOffset;
    result.nLevel=nLevel;
    result.dir_entry=read_IMAGE_RESOURCE_DIRECTORY_ENTRY(nOffset);
    result.rin=getResourcesIDName(nResourceOffset,result.dir_entry.Name);
    result.bIsDataDirectory=result.dir_entry.DataIsDirectory;

    if(result.bIsDataDirectory)
    {
        qint64 nDirectoryOffset=nResourceOffset+result.dir_entry.OffsetToDirectory;
        result.directory=read_IMAGE_RESOURCE_DIRECTORY(nDirectoryOffset);
        nDirectoryOffset+=sizeof(XPE_DEF::IMAGE_RESOURCE_DIRECTORY);

        if((result.directory.NumberOfIdEntries+result.directory.NumberOfNamedEntries<=100)&&(result.directory.Characteristics==0)) // check corrupted
        {
            result.bIsValid=true;

            for(int i=0; i<result.directory.NumberOfIdEntries+result.directory.NumberOfNamedEntries; i++)
            {
                RESOURCE_POSITION rp=_getResourcePosition(pMemoryMap,nBaseAddress,nResourceOffset,nDirectoryOffset,nLevel+1);

                if(!rp.bIsValid)
                {
                    break;
                }

                result.listPositions.append(rp);
                nDirectoryOffset+=sizeof(XPE_DEF::IMAGE_RESOURCE_DIRECTORY_ENTRY);
            }
        }
    }
    else
    {
        result.bIsValid=true;
        result.data_entry=read_IMAGE_RESOURCE_DATA_ENTRY(nResourceOffset+result.dir_entry.OffsetToData);
        result.nDataAddress=nBaseAddress+result.data_entry.OffsetToData;
        result.nDataOffset=addressToOffset(pMemoryMap,result.nDataAddress);
    }

    return result;

    //        nOffsetLevel[0]=nResourceOffset;
    //        rd[0]=read_XPE_DEF::IMAGE_RESOURCE_DIRECTORY(nOffsetLevel[0]);

    //        if((rd[0].NumberOfIdEntries+rd[0].NumberOfNamedEntries<=100)&&(rd[0].Characteristics==0)) // check corrupted
    //        {
    //            nOffsetLevel[0]+=sizeof(XPE_DEF::IMAGE_RESOURCE_DIRECTORY);

    //            for(int i=0; i<rd[0].NumberOfIdEntries+rd[0].NumberOfNamedEntries; i++)
    //            {
    //                rde[0]=read_XPE_DEF::IMAGE_RESOURCE_DIRECTORY_ENTRY(nOffsetLevel[0]);

    //                irin[0]=getResourcesIDName(nResourceOffset,rde[0].Name);
    //                record.nID[0]=irin[0].nID;
    //                record.sName[0]=irin[0].sName;
    //                record.nNameOffset[0]=irin[0].nNameOffset;

    //                nOffsetLevel[1]=nResourceOffset+rde[0].OffsetToDirectory;

    //                rd[1]=read_XPE_DEF::IMAGE_RESOURCE_DIRECTORY(nOffsetLevel[1]);

    //                if(rd[1].Characteristics!=0)
    //                {
    //                    break;
    //                }

    //                nOffsetLevel[1]+=sizeof(XPE_DEF::IMAGE_RESOURCE_DIRECTORY);

    //                for(int j=0; j<rd[1].NumberOfIdEntries+rd[1].NumberOfNamedEntries; j++)
    //                {
    //                    rde[1]=read_XPE_DEF::IMAGE_RESOURCE_DIRECTORY_ENTRY(nOffsetLevel[1]);

    //                    irin[1]=getResourcesIDName(nResourceOffset,rde[1].Name);
    //                    record.nID[1]=irin[1].nID;
    //                    record.sName[1]=irin[1].sName;
    //                    record.nNameOffset[1]=irin[1].nNameOffset;

    //                    nOffsetLevel[2]=nResourceOffset+rde[1].OffsetToDirectory;

    //                    rd[2]=read_XPE_DEF::IMAGE_RESOURCE_DIRECTORY(nOffsetLevel[2]);

    //                    if(rd[2].Characteristics!=0)
    //                    {
    //                        break;
    //                    }

    //                    nOffsetLevel[2]+=sizeof(XPE_DEF::IMAGE_RESOURCE_DIRECTORY);

    //                    for(int k=0; k<rd[2].NumberOfIdEntries+rd[2].NumberOfNamedEntries; k++)
    //                    {
    //                        rde[2]=read_XPE_DEF::IMAGE_RESOURCE_DIRECTORY_ENTRY(nOffsetLevel[2]);

    //                        irin[2]=getResourcesIDName(nResourceOffset,rde[2].Name);
    //                        record.nID[2]=irin[2].nID;
    //                        record.sName[2]=irin[2].sName;
    //                        record.nNameOffset[2]=irin[2].nNameOffset;

    //                        record.nIRDEOffset=rde[2].OffsetToData;
    //                        XPE_DEF::IMAGE_RESOURCE_DATA_ENTRY irde=read_XPE_DEF::IMAGE_RESOURCE_DATA_ENTRY(nResourceOffset+record.nIRDEOffset);
    //                        record.nRVA=irde.OffsetToData;
    //                        record.nAddress=irde.OffsetToData+nBaseAddress;
    //                        record.nOffset=addressToOffset(&memoryMap,record.nAddress);
    //                        record.nSize=irde.Size;

    //                        listResources.append(record);

    //                        nOffsetLevel[2]+=sizeof(XPE_DEF::IMAGE_RESOURCE_DIRECTORY_ENTRY);
    //                    }

    //                    nOffsetLevel[1]+=sizeof(XPE_DEF::IMAGE_RESOURCE_DIRECTORY_ENTRY);
    //                }

    //                nOffsetLevel[0]+=sizeof(XPE_DEF::IMAGE_RESOURCE_DIRECTORY_ENTRY);
    //            }
    //        }
}

void XPE::_fixCheckSum()
{
    setOptionalHeader_CheckSum(calculateCheckSum());
}

QList<XPE_DEF::IMAGE_SECTION_HEADER> XPE::splitSection(QByteArray *pbaData, XPE_DEF::IMAGE_SECTION_HEADER shOriginal, quint32 nBlockSize)
{
    QList<XPE_DEF::IMAGE_SECTION_HEADER> listResult;
    //    int nBlockSize=0x1000;
    int nSize=pbaData->size();
    char *pOffset=pbaData->data();
    char *pOffsetStart=pOffset;
    int nCount=nSize/nBlockSize;
    quint64 nVirtualAddress=shOriginal.VirtualAddress;
    qint64 nRelVirtualStart=0;
    qint64 nRelVirtualEnd=S_ALIGN_UP(shOriginal.Misc.VirtualSize,nBlockSize);
    qint64 nRelCurrent=nRelVirtualStart;

    if(nCount>1)
    {
        // Check the first block
        while(isEmptyData(pOffset,nBlockSize))
        {
            pOffset+=nBlockSize;
            nRelCurrent+=nBlockSize;
            nCount--;

            if(pOffset>=pOffsetStart+nSize)
            {
                break;
            }
        }

        if(pOffset!=pOffsetStart)
        {
            XPE_DEF::IMAGE_SECTION_HEADER sh=shOriginal;
            sh.VirtualAddress=nVirtualAddress;
            //            sh.Misc.VirtualSize=pOffset-pOffsetStart;
            sh.Misc.VirtualSize=nRelCurrent-nRelVirtualStart;
            sh.SizeOfRawData=XBinary::getPhysSize(pOffsetStart,sh.Misc.VirtualSize);
            listResult.append(sh);

            nVirtualAddress+=sh.Misc.VirtualSize;
        }

        bool bNew=false;
        pOffsetStart=pOffset;
        nRelVirtualStart=nRelCurrent;

        while(nCount>0)
        {
            if(isEmptyData(pOffset,nBlockSize))
            {
                bNew=true;
            }
            else
            {
                if(bNew)
                {
                    XPE_DEF::IMAGE_SECTION_HEADER sh=shOriginal;
                    sh.VirtualAddress=nVirtualAddress;
                    //                    sh.Misc.VirtualSize=pOffset-pOffsetStart;
                    sh.Misc.VirtualSize=nRelCurrent-nRelVirtualStart;
                    sh.SizeOfRawData=XBinary::getPhysSize(pOffsetStart,sh.Misc.VirtualSize);
                    listResult.append(sh);

                    nVirtualAddress+=sh.Misc.VirtualSize;

                    pOffsetStart=pOffset;
                    nRelVirtualStart=nRelCurrent;
                    bNew=false;
                }
            }

            pOffset+=nBlockSize;
            nRelCurrent+=nBlockSize;
            nCount--;
        }

        if(pOffset!=pOffsetStart)
        {
            XPE_DEF::IMAGE_SECTION_HEADER sh=shOriginal;
            sh.VirtualAddress=nVirtualAddress;
            //            sh.Misc.VirtualSize=pOffset-pOffsetStart;
            sh.Misc.VirtualSize=nRelVirtualEnd-nRelVirtualStart;
            sh.SizeOfRawData=XBinary::getPhysSize(pOffsetStart,nSize-(pOffsetStart-pbaData->data()));

            if(sh.Misc.VirtualSize)
            {
                listResult.append(sh);
            }

            nVirtualAddress+=sh.Misc.VirtualSize;
        }
    }
    else
    {
        listResult.append(shOriginal);
    }

    return listResult;
}

QByteArray XPE::createHeaderStub(HEADER_OPTIONS *pHeaderOptions) // TODO options
{
    QByteArray baResult;

    baResult.resize(0x200);
    baResult.fill(0);

    QBuffer buffer(&baResult);

    if(buffer.open(QIODevice::ReadWrite))
    {
        XPE pe(&buffer);

        pe.set_e_magic(XMSDOS_DEF::S_IMAGE_DOS_SIGNATURE);
        pe.set_e_lfanew(0x40);
        pe.setNtHeaders_Signature(XPE_DEF::S_IMAGE_NT_SIGNATURE);
        pe.setFileHeader_SizeOfOptionalHeader(0xE0); // TODO
        pe.setFileHeader_Machine(pHeaderOptions->nMachine);
        pe.setFileHeader_Characteristics(pHeaderOptions->nCharacteristics);
        pe.setOptionalHeader_Magic(pHeaderOptions->nMagic);
        pe.setOptionalHeader_ImageBase(pHeaderOptions->nImagebase);
        pe.setOptionalHeader_DllCharacteristics(pHeaderOptions->nDllcharacteristics);
        pe.setOptionalHeader_Subsystem(pHeaderOptions->nSubsystem);
        pe.setOptionalHeader_MajorOperatingSystemVersion(pHeaderOptions->nMajorOperationSystemVersion);
        pe.setOptionalHeader_MinorOperatingSystemVersion(pHeaderOptions->nMinorOperationSystemVersion);
        pe.setOptionalHeader_FileAlignment(pHeaderOptions->nFileAlignment);
        pe.setOptionalHeader_SectionAlignment(pHeaderOptions->nSectionAlignment);
        pe.setOptionalHeader_AddressOfEntryPoint(pHeaderOptions->nAddressOfEntryPoint);
        pe.setOptionalHeader_NumberOfRvaAndSizes(0x10);

        pe.setOptionalHeader_DataDirectory_VirtualAddress(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_RESOURCE,pHeaderOptions->nResourceRVA);
        pe.setOptionalHeader_DataDirectory_Size(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_RESOURCE,pHeaderOptions->nResourceSize);

        buffer.close();
    }

    return baResult;
}

qint64 XPE::_calculateHeadersSize(qint64 nSectionsTableOffset, quint32 nNumberOfSections)
{
    qint64 nHeadersSize=nSectionsTableOffset+sizeof(XPE_DEF::IMAGE_SECTION_HEADER)*nNumberOfSections;
    quint32 nFileAlignment=getOptionalHeader_FileAlignment();
    nHeadersSize=S_ALIGN_UP(nHeadersSize,nFileAlignment);

    return nHeadersSize;
}

bool XPE::isDll()
{
    bool bResult=false;

    if(getOptionalHeader_Subsystem()!=XPE_DEF::S_IMAGE_SUBSYSTEM_NATIVE)
    {
        bResult=(getFileHeader_Characteristics()&XPE_DEF::S_IMAGE_FILE_DLL);
    }

    return bResult;
}

bool XPE::isConsole()
{
    return (getOptionalHeader_Subsystem()==XPE_DEF::S_IMAGE_SUBSYSTEM_WINDOWS_CUI);
}

bool XPE::isNETPresent()
{
    // TODO more checks
    return isOptionalHeader_DataDirectoryPresent(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR);
}

XPE::CLI_INFO XPE::getCliInfo(bool bFindHidden)
{
    CLI_INFO result= {};

    if(isNETPresent()||bFindHidden)
    {
        QList<MEMORY_MAP> listMM=getMemoryMapList();
        qint64 nBaseAddress=_getBaseAddress();

        qint64 nCLIHeaderOffset=-1;

        if(isNETPresent())
        {
            XPE_DEF::IMAGE_DATA_DIRECTORY _idd=getOptionalHeader_DataDirectory(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR);

            nCLIHeaderOffset=addressToOffset(&listMM,nBaseAddress+_idd.VirtualAddress);
        }
        else
        {
            // mb TODO
            nCLIHeaderOffset=addressToOffset(&listMM,nBaseAddress+0x2008);
            result.bHidden=true;
        }

        if(nCLIHeaderOffset!=-1)
        {
            result.nCLIHeaderOffset=nCLIHeaderOffset;

            read_array(result.nCLIHeaderOffset,(char *)&(result.header),sizeof(XPE_DEF::IMAGE_COR20_HEADER));

            if((result.header.cb==0x48)&&result.header.MetaData.VirtualAddress&&result.header.MetaData.Size)
            {
                result.nEntryPointSize=0;
                result.nEntryPoint=result.header.EntryPointRVA;

                result.nCLI_MetaDataOffset=addressToOffset(&listMM,nBaseAddress+result.header.MetaData.VirtualAddress);

                if(result.nCLI_MetaDataOffset!=-1)
                {
                    result.nCLI_MetaData_Signature=read_uint32(result.nCLI_MetaDataOffset);

                    if(result.nCLI_MetaData_Signature==0x424a5342)
                    {
                        result.bInit=true;

                        result.sCLI_MetaData_MajorVersion=read_uint16(result.nCLI_MetaDataOffset+4);
                        result.sCLI_MetaData_MinorVersion=read_uint16(result.nCLI_MetaDataOffset+6);
                        result.nCLI_MetaData_Reserved=read_uint32(result.nCLI_MetaDataOffset+8);
                        result.nCLI_MetaData_VersionStringLength=read_uint32(result.nCLI_MetaDataOffset+12);

                        result.sCLI_MetaData_Version=read_ansiString(result.nCLI_MetaDataOffset+16,result.nCLI_MetaData_VersionStringLength);
                        result.sCLI_MetaData_Flags=read_uint16(result.nCLI_MetaDataOffset+16+result.nCLI_MetaData_VersionStringLength);
                        result.sCLI_MetaData_Streams=read_uint16(result.nCLI_MetaDataOffset+16+result.nCLI_MetaData_VersionStringLength+2);


                        qint64 nOffset=result.nCLI_MetaDataOffset+20+result.nCLI_MetaData_VersionStringLength;

                        for(int i=0; i<result.sCLI_MetaData_Streams; i++)
                        {
                            result.listCLI_MetaData_Stream_Offsets.append(read_uint32(nOffset));
                            result.listCLI_MetaData_Stream_Sizes.append(read_uint32(nOffset+4));
                            result.listCLI_MetaData_Stream_Names.append(read_ansiString(nOffset+8));

                            if(result.listCLI_MetaData_Stream_Names.at(i)=="#~")
                            {
                                result.nCLI_MetaData_TablesHeaderOffset=result.listCLI_MetaData_Stream_Offsets.at(i)+result.nCLI_MetaDataOffset;
                                result.nCLI_MetaData_TablesSize=result.listCLI_MetaData_Stream_Sizes.at(i);
                            }
                            else if(result.listCLI_MetaData_Stream_Names.at(i)=="#Strings")
                            {
                                result.nCLI_MetaData_StringsOffset=result.listCLI_MetaData_Stream_Offsets.at(i)+result.nCLI_MetaDataOffset;
                                result.nCLI_MetaData_StringsSize=result.listCLI_MetaData_Stream_Sizes.at(i);

                                QByteArray baStrings=read_array(result.nCLI_MetaData_StringsOffset,result.nCLI_MetaData_StringsSize);

                                char *_pOffset=baStrings.data();
                                int _nSize=baStrings.size();

                                for(int i=1; i<_nSize; i++)
                                {
                                    _pOffset++;
                                    QString sTemp=_pOffset;
                                    result.listAnsiStrings.append(sTemp);

                                    _pOffset+=sTemp.size();
                                    i+=sTemp.size();
                                }
                            }
                            else if(result.listCLI_MetaData_Stream_Names.at(i)=="#US")
                            {
                                result.nCLI_MetaData_USOffset=result.listCLI_MetaData_Stream_Offsets.at(i)+result.nCLI_MetaDataOffset;
                                result.nCLI_MetaData_USSize=result.listCLI_MetaData_Stream_Sizes.at(i);

                                QByteArray baStrings=read_array(result.nCLI_MetaData_USOffset,result.nCLI_MetaData_USSize);

                                char *_pOffset=baStrings.data();
                                char *__pOffset=_pOffset;
                                int _nSize=baStrings.size();

                                __pOffset++;

                                for(int i=1; i<_nSize; i++)
                                {
                                    int nStringSize=(*((unsigned char *)__pOffset));

                                    if(nStringSize==0x80)
                                    {
                                        nStringSize=0;
                                    }

                                    if(nStringSize>_nSize-i)
                                    {
                                        break;
                                    }

                                    __pOffset++;

                                    if(__pOffset>_pOffset+_nSize)
                                    {
                                        break;
                                    }

                                    QString sTemp=QString::fromUtf16((ushort *)__pOffset,nStringSize/2);

                                    result.listUnicodeStrings.append(sTemp);

                                    __pOffset+=nStringSize;
                                    i+=nStringSize;
                                }
                            }
                            else if(result.listCLI_MetaData_Stream_Names.at(i)=="#Blob")
                            {
                                result.nCLI_MetaData_BlobOffset=result.listCLI_MetaData_Stream_Offsets.at(i)+result.nCLI_MetaDataOffset;
                                result.nCLI_MetaData_BlobSize=result.listCLI_MetaData_Stream_Sizes.at(i);
                            }
                            else if(result.listCLI_MetaData_Stream_Names.at(i)=="#GUID")
                            {
                                result.nCLI_MetaData_GUIDOffset=result.listCLI_MetaData_Stream_Offsets.at(i)+result.nCLI_MetaDataOffset;
                                result.nCLI_MetaData_GUIDSize=result.listCLI_MetaData_Stream_Sizes.at(i);
                            }

                            nOffset+=8;
                            nOffset+=S_ALIGN_UP((result.listCLI_MetaData_Stream_Names.at(i).length()+1),4);
                        }

                        if(result.nCLI_MetaData_TablesHeaderOffset)
                        {
                            result.nCLI_MetaData_Tables_Reserved1=read_uint32(result.nCLI_MetaData_TablesHeaderOffset);
                            result.cCLI_MetaData_Tables_MajorVersion=read_uint8(result.nCLI_MetaData_TablesHeaderOffset+4);
                            result.cCLI_MetaData_Tables_MinorVersion=read_uint8(result.nCLI_MetaData_TablesHeaderOffset+5);
                            result.cCLI_MetaData_Tables_HeapOffsetSizes=read_uint8(result.nCLI_MetaData_TablesHeaderOffset+6);
                            result.cCLI_MetaData_Tables_Reserved2=read_uint8(result.nCLI_MetaData_TablesHeaderOffset+7);
                            result.nCLI_MetaData_Tables_Valid=read_uint64(result.nCLI_MetaData_TablesHeaderOffset+8);
                            result.nCLI_MetaData_Tables_Sorted=read_uint64(result.nCLI_MetaData_TablesHeaderOffset+16);

                            unsigned long long nValid=result.nCLI_MetaData_Tables_Valid;

                            unsigned int nTemp=0;

                            for(nTemp = 0; nValid; nTemp++)
                            {
                                nValid &= nValid - 1;
                            }

                            result.nCLI_MetaData_Tables_Valid_NumberOfRows=nTemp;

                            nOffset=result.nCLI_MetaData_TablesHeaderOffset+24;

                            for(int i=0; i<64; i++)
                            {
                                if(result.nCLI_MetaData_Tables_Valid&((unsigned long long)1<<i))
                                {
                                    result.CLI_MetaData_Tables_TablesNumberOfIndexes[i]=read_uint32(nOffset);
                                    nOffset+=4;
                                }
                            }

                            unsigned int nSize=0;
                            int nStringIndexSize=2;
                            int nGUIDIndexSize=2;
                            int nBLOBIndexSize=2;
                            int nResolutionScope=2;
                            int nTypeDefOrRef=2;
                            int nField=2;
                            int nMethodDef=2;
                            int nParamList=2;

                            unsigned char cHeapOffsetSizes=0;

                            cHeapOffsetSizes=result.cCLI_MetaData_Tables_HeapOffsetSizes;

                            if(cHeapOffsetSizes&0x01)
                            {
                                nStringIndexSize=4;
                            }

                            if(cHeapOffsetSizes&0x02)
                            {
                                nGUIDIndexSize=4;
                            }

                            if(cHeapOffsetSizes&0x04)
                            {
                                nBLOBIndexSize=4;
                            }

                            // TODO !!!
                            if(result.CLI_MetaData_Tables_TablesNumberOfIndexes[0]>0x3FFF)
                            {
                                nResolutionScope=4;
                            }

                            if(result.CLI_MetaData_Tables_TablesNumberOfIndexes[26]>0x3FFF)
                            {
                                nResolutionScope=4;
                            }

                            if(result.CLI_MetaData_Tables_TablesNumberOfIndexes[35]>0x3FFF)
                            {
                                nResolutionScope=4;
                            }

                            if(result.CLI_MetaData_Tables_TablesNumberOfIndexes[1]>0x3FFF)
                            {
                                nResolutionScope=4;
                            }

                            if(result.CLI_MetaData_Tables_TablesNumberOfIndexes[1]>0x3FFF)
                            {
                                nTypeDefOrRef=4;
                            }

                            if(result.CLI_MetaData_Tables_TablesNumberOfIndexes[2]>0x3FFF)
                            {
                                nTypeDefOrRef=4;
                            }

                            if(result.CLI_MetaData_Tables_TablesNumberOfIndexes[27]>0x3FFF)
                            {
                                nTypeDefOrRef=4;
                            }

                            if(result.CLI_MetaData_Tables_TablesNumberOfIndexes[4]>0xFFFF)
                            {
                                nField=4;
                            }

                            if(result.CLI_MetaData_Tables_TablesNumberOfIndexes[6]>0xFFFF)
                            {
                                nMethodDef=4;
                            }

                            if(result.CLI_MetaData_Tables_TablesNumberOfIndexes[8]>0xFFFF)
                            {
                                nParamList=4;
                            }

                            nSize=0;
                            nSize+=2;
                            nSize+=nStringIndexSize;
                            nSize+=nGUIDIndexSize;
                            nSize+=nGUIDIndexSize;
                            nSize+=nGUIDIndexSize;
                            result.CLI_MetaData_Tables_TablesSizes[0]=nSize;
                            nSize=0;
                            nSize+=nResolutionScope;
                            nSize+=nStringIndexSize;
                            nSize+=nStringIndexSize;
                            result.CLI_MetaData_Tables_TablesSizes[1]=nSize;
                            nSize=0;
                            nSize+=4;
                            nSize+=nStringIndexSize;
                            nSize+=nStringIndexSize;
                            nSize+=nTypeDefOrRef;
                            nSize+=nField;
                            nSize+=nMethodDef;
                            result.CLI_MetaData_Tables_TablesSizes[2]=nSize;
                            nSize=0;
                            result.CLI_MetaData_Tables_TablesSizes[3]=nSize;
                            nSize=0;
                            nSize+=2;
                            nSize+=nStringIndexSize;
                            nSize+=nBLOBIndexSize;
                            result.CLI_MetaData_Tables_TablesSizes[4]=nSize;
                            nSize=0;
                            result.CLI_MetaData_Tables_TablesSizes[5]=nSize;
                            nSize=0;
                            nSize+=4;
                            nSize+=2;
                            nSize+=2;
                            nSize+=nStringIndexSize;
                            nSize+=nBLOBIndexSize;
                            nSize+=nParamList;
                            result.CLI_MetaData_Tables_TablesSizes[6]=nSize;

                            for(int i=0; i<64; i++)
                            {
                                if(result.CLI_MetaData_Tables_TablesNumberOfIndexes[i])
                                {
                                    result.CLI_MetaData_Tables_TablesOffsets[i]=nOffset;
                                    nOffset+=result.CLI_MetaData_Tables_TablesSizes[i]*result.CLI_MetaData_Tables_TablesNumberOfIndexes[i];
                                }
                            }

                            if(!(result.header.Flags&XPE_DEF::COMIMAGE_FLAGS_NATIVE_ENTRYPOINT))
                            {
                                if(((result.nEntryPoint&0xFF000000)>>24)==6)
                                {
                                    unsigned int nIndex=result.nEntryPoint&0xFFFFFF;

                                    if(nIndex<=result.CLI_MetaData_Tables_TablesNumberOfIndexes[6])
                                    {
                                        nOffset=result.CLI_MetaData_Tables_TablesOffsets[6];
                                        nOffset+=result.CLI_MetaData_Tables_TablesSizes[6]*(nIndex-1);

                                        result.nEntryPoint=read_uint32(nOffset);
                                    }
                                    else
                                    {
                                        result.nEntryPoint=0;
                                    }
                                }
                                else
                                {
                                    result.nEntryPoint=0;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    //    emit appendError(".NET is not present");
    return result;
}

bool XPE::isNETAnsiStringPresent(QString sString, XPE::CLI_INFO *pCliInfo)
{
    bool bResult=false;

    for(int i=0; i<pCliInfo->listAnsiStrings.count(); i++)
    {
        QString sRecord=pCliInfo->listAnsiStrings.at(i);

        if(sRecord!="")
        {
            if(sString==sRecord)
            {
                bResult=true;
                break;
            }
        }
    }

    return bResult;
}

int XPE::getEntryPointSection()
{
    int nResult=-1;

    qint64 nAddressOfEntryPoint=getOptionalHeader_AddressOfEntryPoint();

    if(nAddressOfEntryPoint)
    {
        nResult=addressToSection(_getBaseAddress()+nAddressOfEntryPoint);
    }

    return nResult;
}

int XPE::getImportSection()
{
    int nResult=-1;

    qint64 nAddressOfImport=getOptionalHeader_DataDirectory(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_IMPORT).VirtualAddress;

    if(nAddressOfImport)
    {
        nResult=addressToSection(_getBaseAddress()+nAddressOfImport);
    }

    return nResult;
}

int XPE::getExportSection()
{
    int nResult=-1;

    qint64 nAddressOfExport=getOptionalHeader_DataDirectory(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_EXPORT).VirtualAddress;

    if(nAddressOfExport)
    {
        nResult=addressToSection(_getBaseAddress()+nAddressOfExport);
    }

    return nResult;
}

int XPE::getTLSSection()
{
    int nResult=-1;

    qint64 nAddressOfTLS=getOptionalHeader_DataDirectory(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_TLS).VirtualAddress;

    if(nAddressOfTLS)
    {
        nResult=addressToSection(_getBaseAddress()+nAddressOfTLS);
    }

    return nResult;
}

int XPE::getResourcesSection()
{
    int nResult=-1;

    qint64 nAddressOfResources=getOptionalHeader_DataDirectory(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_RESOURCE).VirtualAddress;

    if(nAddressOfResources)
    {
        nResult=addressToSection(_getBaseAddress()+nAddressOfResources);
    }

    return nResult;
}

int XPE::getRelocsSection()
{
    int nResult=-1;

    qint64 nAddressOfRelocs=getOptionalHeader_DataDirectory(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_BASERELOC).VirtualAddress;

    if(nAddressOfRelocs)
    {
        nResult=addressToSection(_getBaseAddress()+nAddressOfRelocs);
    }

    return nResult;
}

int XPE::getNormalCodeSection()
{
    int nResult=-1;
    // TODO opimize

    QList<XPE_DEF::IMAGE_SECTION_HEADER> listSections=getSectionHeaders();
    int nNumberOfSections=listSections.count();
    nNumberOfSections=qMin(nNumberOfSections,2);

    for(int i=0; i<nNumberOfSections; i++)
    {
        QString sSectionName=QString((char *)listSections.at(i).Name);
        sSectionName.resize(qMin(sSectionName.length(),XPE_DEF::S_IMAGE_SIZEOF_SHORT_NAME));
        quint32 nSectionCharacteristics=listSections.at(i).Characteristics;
        nSectionCharacteristics&=0xFF0000FF;

        // .textbss
        // 0x60500060 mingw
        if((((sSectionName=="CODE")||sSectionName==".text"))&&
                (nSectionCharacteristics==0x60000020)&&
                (listSections.at(i).SizeOfRawData))
        {
            nResult=addressToSection(_getBaseAddress()+listSections.at(i).VirtualAddress);
            break;
        }
    }

    if(nResult==-1)
    {
        if(nNumberOfSections>0)
        {
            if(listSections.at(0).SizeOfRawData)
            {
                nResult=addressToSection(_getBaseAddress()+listSections.at(0).VirtualAddress);
            }
        }
    }

    return nResult;
}

int XPE::getNormalDataSection()
{
    int nResult=-1;
    // TODO opimize

    QList<XPE_DEF::IMAGE_SECTION_HEADER> listSections=getSectionHeaders();
    int nNumberOfSections=listSections.count();

    int nImportSection=getImportSection();

    for(int i=1; i<nNumberOfSections; i++)
    {
        // 0xc0700040 MinGW
        // 0xc0600040 MinGW
        // 0xc0300040 MinGW
        QString sSectionName=QString((char *)listSections.at(i).Name);
        sSectionName.resize(qMin(sSectionName.length(),XPE_DEF::S_IMAGE_SIZEOF_SHORT_NAME));
        quint32 nSectionCharacteristics=listSections.at(i).Characteristics;
        nSectionCharacteristics&=0xFF0000FF;

        if((((sSectionName=="DATA")||sSectionName==".data"))&&
                (nSectionCharacteristics==0xC0000040)&&
                (listSections.at(i).SizeOfRawData)&&
                (nImportSection!=i))
        {
            nResult=addressToSection(_getBaseAddress()+listSections.at(i).VirtualAddress);
            break;
        }
    }

    if(nResult==-1)
    {
        for(int i=1; i<nNumberOfSections; i++)
        {
            if(listSections.at(i).SizeOfRawData&&(nImportSection!=i)&&
                    (listSections.at(i).Characteristics!=0x60000020)&&
                    (listSections.at(i).Characteristics!=0x40000040))
            {
                nResult=addressToSection(_getBaseAddress()+listSections.at(i).VirtualAddress);
                break;
            }
        }
    }

    return nResult;
}

int XPE::getConstDataSection()
{
    int nResult=-1;
    // TODO opimize

    QList<XPE_DEF::IMAGE_SECTION_HEADER> listSections=getSectionHeaders();
    int nNumberOfSections=listSections.count();

    for(int i=1; i<nNumberOfSections; i++)
    {
        // 0x40700040 MinGW
        // 0x40600040 MinGW
        // 0x40300040 MinGW
        QString sSectionName=QString((char *)listSections.at(i).Name);
        sSectionName.resize(qMin(sSectionName.length(),XPE_DEF::S_IMAGE_SIZEOF_SHORT_NAME));
        quint32 nSectionCharacteristics=listSections.at(i).Characteristics;
        nSectionCharacteristics&=0xFF0000FF;

        if((sSectionName==".rdata")&&
                (nSectionCharacteristics==0x40000040)&&
                (listSections.at(i).SizeOfRawData))
        {
            nResult=addressToSection(_getBaseAddress()+listSections.at(i).VirtualAddress);
            break;
        }
    }

    return nResult;
}

bool XPE::rebuildDump(QString sResultFile,REBUILD_OPTIONS *pRebuildOptions)
{
    // TODO rework!
#ifdef QT_DEBUG
    QElapsedTimer timer;
    timer.start();
    qDebug("QPE::rebuildDump");
#endif

    bool bResult=false;

    if(sResultFile!="")
    {
        quint32 nTotalSize=0;
        quint32 nHeaderSize=0;
        QList<quint32> listSectionsSize;
        QList<quint32> listSectionsOffsets;

        quint32 nFileAlignment=getOptionalHeader_FileAlignment();

        quint32 nSectionAlignment=getOptionalHeader_SectionAlignment();

        if(pRebuildOptions->bOptimize)
        {
            QByteArray baHeader=getHeaders();
            int nNumberOfSections=getFileHeader_NumberOfSections();

            if(pRebuildOptions->bClearHeader)
            {
                nHeaderSize=getSectionsTableOffset()+nNumberOfSections*sizeof(XPE_DEF::IMAGE_SECTION_HEADER);
            }
            else
            {
                nHeaderSize=XBinary::getPhysSize(baHeader.data(),baHeader.size());
            }

            nHeaderSize=XBinary::getPhysSize(baHeader.data(),baHeader.size());

            for(int i=0; i<nNumberOfSections; i++)
            {
                QByteArray baSection=read_array(getSection_VirtualAddress(i),getSection_VirtualSize(i));
                quint32 nSectionSize=XBinary::getPhysSize(baSection.data(),baSection.size());
                listSectionsSize.append(nSectionSize);
            }

            nTotalSize+=S_ALIGN_UP(nHeaderSize,nFileAlignment);

            for(int i=0; i<listSectionsSize.size(); i++)
            {
                listSectionsOffsets.append(nTotalSize);

                if(listSectionsSize.at(i))
                {
                    nTotalSize+=S_ALIGN_UP(listSectionsSize.at(i),nFileAlignment);
                }
            }
        }
        else
        {
            nTotalSize=getSize();
        }

#ifdef QT_DEBUG
        qDebug("QPE::rebuildDump:totalsize: %lld msec",timer.elapsed());
#endif

        QByteArray baBuffer;
        baBuffer.resize(nTotalSize);
        baBuffer.fill(0);
        QBuffer buffer;
        buffer.setBuffer(&baBuffer);

        if(buffer.open(QIODevice::ReadWrite))
        {
            XPE bufPE(&buffer,false);

            if(pRebuildOptions->bOptimize)
            {
                XBinary::copyDeviceMemory(getDevice(),0,&buffer,0,nHeaderSize);
                bufPE.setOptionalHeader_SizeOfHeaders(S_ALIGN_UP(nHeaderSize,nFileAlignment));
            }
            else
            {
                XBinary::copyDeviceMemory(getDevice(),0,&buffer,0,nTotalSize);
            }

#ifdef QT_DEBUG
            qDebug("QPE::rebuildDump:copy: %lld msec",timer.elapsed());
#endif

            int nNumberOfSections=getFileHeader_NumberOfSections();

            for(int i=0; i<nNumberOfSections; i++)
            {
                if(pRebuildOptions->bOptimize)
                {
                    XBinary::copyDeviceMemory(getDevice(),getSection_VirtualAddress(i),&buffer,listSectionsOffsets.at(i),listSectionsSize.at(i));
                    bufPE.setSection_PointerToRawData(i,listSectionsOffsets.at(i));
                    bufPE.setSection_SizeOfRawData(i,S_ALIGN_UP(listSectionsSize.at(i),nFileAlignment));
                }
                else
                {
                    quint32 nSectionAddress=getSection_VirtualAddress(i);
                    quint32 nSectionSize=getSection_VirtualSize(i);
                    bufPE.setSection_SizeOfRawData(i,S_ALIGN_UP(nSectionSize,nSectionAlignment));
                    bufPE.setSection_PointerToRawData(i,nSectionAddress);
                }

                bufPE.setSection_Characteristics(i,0xe0000020); // !!!
            }

#ifdef QT_DEBUG
            qDebug("QPE::rebuildDump:copysections: %lld msec",timer.elapsed());
#endif

            bResult=true;

            buffer.close();
        }

        QFile file;
        file.setFileName(sResultFile);

        if(file.open(QIODevice::ReadWrite))
        {
#ifdef QT_DEBUG
            qDebug("QPE::rebuildDump:write:start: %lld msec",timer.elapsed());
#endif
            file.resize(baBuffer.size());
            file.write(baBuffer.data(),baBuffer.size());
            file.close();
#ifdef QT_DEBUG
            qDebug("QPE::rebuildDump:write: %lld msec",timer.elapsed());
#endif

            bResult=true;
        }
    }

    if(bResult)
    {
        bResult=false;

        QFile file;
        file.setFileName(sResultFile);

        if(file.open(QIODevice::ReadWrite))
        {
            XPE _pe(&file,false);

            if(_pe.isValid())
            {
                //                if(pRebuildOptions->bRemoveLastSection)
                //                {
                //                    _pe.removeLastSection();
                //                }
                //            #ifdef QT_DEBUG
                //                qDebug("QPE::rebuildDump:removelastsection: %lld msec",timer.elapsed());
                //            #endif
                if(!pRebuildOptions->mapPatches.empty())
                {
                    QList<MEMORY_MAP> listMemoryMap=_pe.getMemoryMapList();

                    QMapIterator<qint64, quint64> i(pRebuildOptions->mapPatches);

                    while(i.hasNext())
                    {
                        i.next();

                        qint64 nAddress=i.key();
                        quint64 nValue=i.value();

                        quint64 nOffset=_pe.addressToOffset(&listMemoryMap,nAddress);

                        if(_pe.is64())
                        {
                            _pe.write_uint64(nOffset,nValue);
                        }
                        else
                        {
                            _pe.write_uint32(nOffset,(quint32)nValue);
                        }
                    }
                }

#ifdef QT_DEBUG
                qDebug("QPE::rebuildDump:mapPatches: %lld msec",timer.elapsed());
#endif

                if(pRebuildOptions->bSetEntryPoint)
                {
                    _pe.setOptionalHeader_AddressOfEntryPoint(pRebuildOptions->nEntryPoint);
                }

#ifdef QT_DEBUG
                qDebug("QPE::rebuildDump:setentrypoint: %lld msec",timer.elapsed());
#endif

                if(!pRebuildOptions->mapIAT.isEmpty())
                {
                    if(!_pe.addImportSection(&(pRebuildOptions->mapIAT)))
                    {
                        _errorMessage(tr("Cannot add import section"));
                    }
                }

#ifdef QT_DEBUG
                qDebug("QPE::rebuildDump:addimportsection: %lld msec",timer.elapsed());
#endif

                if(pRebuildOptions->bRenameSections)
                {
                    int nNumberOfSections=_pe.getFileHeader_NumberOfSections();

                    for(int i=0; i<nNumberOfSections; i++)
                    {
                        QString sSection=_pe.getSection_NameAsString(i);

                        if(sSection!=".rsrc")
                        {
                            _pe.setSection_NameAsString(i,pRebuildOptions->sSectionName);
                        }
                    }
                }

#ifdef QT_DEBUG
                qDebug("QPE::rebuildDump:renamesections: %lld msec",timer.elapsed());
#endif

                if(pRebuildOptions->listRelocsRVAs.count())
                {
                    _pe.addRelocsSection(&(pRebuildOptions->listRelocsRVAs));
                }

#ifdef QT_DEBUG
                qDebug("QPE::rebuildDump:addrelocssection: %lld msec",timer.elapsed());
#endif

                if(pRebuildOptions->bFixChecksum)
                {
                    _pe._fixCheckSum();
                }

#ifdef QT_DEBUG
                qDebug("QPE::rebuildDump:fixchecksum: %lld msec",timer.elapsed());
#endif
            }

            bResult=true;

            file.close();
        }
    }

#ifdef QT_DEBUG
    qDebug("QPE::rebuildDump: %lld msec",timer.elapsed());
#endif

    return bResult;
}

bool XPE::rebuildDump(QString sInputFile, QString sResultFile,REBUILD_OPTIONS *pRebuildOptions)
{
    // TODO rework!
    bool bResult=false;
    QFile file;
    file.setFileName(sInputFile);

    if(file.open(QIODevice::ReadOnly))
    {
        XPE pe(&file,false);

        if(pe.isValid())
        {
            bResult=pe.rebuildDump(sResultFile,pRebuildOptions);
        }

        file.close();
    }

    return bResult;
}

bool XPE::fixCheckSum(QString sFileName, bool bIsImage)
{
    bool bResult=false;
    QFile file;
    file.setFileName(sFileName);

    if(file.open(QIODevice::ReadWrite))
    {
        XPE pe(&file,bIsImage);

        if(pe.isValid())
        {
            pe._fixCheckSum();
            bResult=true;
        }

        file.close();
    }

    return bResult;
}

qint64 XPE::_fixHeadersSize()
{
    quint32 nNumberOfSections=getFileHeader_NumberOfSections();
    qint64 nSectionsTableOffset=getSectionsTableOffset();
    qint64 nHeadersSize=_calculateHeadersSize(nSectionsTableOffset,nNumberOfSections);

    // MB TODO
    setOptionalHeader_SizeOfHeaders(nHeadersSize);

    return nHeadersSize;
}

qint64 XPE::_getMinSectionOffset()
{
    qint64 nResult=-1;

    QList<MEMORY_MAP> listMM=getMemoryMapList();

    for(int i=0; i<listMM.count(); i++)
    {
        if(listMM.at(i).bIsLoadSection)
        {
            if(nResult==-1)
            {
                nResult=listMM.at(i).nOffset;
            }
            else
            {
                nResult=qMin(nResult,listMM.at(i).nOffset);
            }
        }
    }

    return nResult;
}

void XPE::_fixFileOffsets(qint64 nDelta)
{
    if(nDelta)
    {
        setOptionalHeader_SizeOfHeaders(getOptionalHeader_SizeOfHeaders()+nDelta);
        quint32 nNumberOfSections=getFileHeader_NumberOfSections();

        for(quint32 i=0; i<nNumberOfSections; i++)
        {
            quint32 nFileOffset=getSection_PointerToRawData(i);
            setSection_PointerToRawData(i,nFileOffset+nDelta);
        }
    }
}

quint16 XPE::_checkSum(qint64 nStartValue,qint64 nDataSize)
{
    // TODO Check
    // TODO Optimize
    const int BUFFER_SIZE=0x1000;
    int nSum=(int)nStartValue;
    unsigned int nTemp=0;
    char *pBuffer=new char[BUFFER_SIZE];
    char *pOffset;

    while(nDataSize>0)
    {
        nTemp=qMin((qint64)BUFFER_SIZE,nDataSize);

        if(!read_array(nStartValue,pBuffer,nTemp))
        {
            delete[] pBuffer;

            return false;
        }

        pOffset=pBuffer;

        for(unsigned int i=0; i<(nTemp+1)/2; i++)
        {
            nSum+=*((unsigned short *)pOffset);

            if(S_HIWORD(nSum)!=0)
            {
                nSum=S_LOWORD(nSum)+S_HIWORD(nSum);
            }

            pOffset+=2;
        }

        nDataSize-=nTemp;
        nStartValue+=nTemp;
    }

    delete[] pBuffer;

    return (unsigned short)(S_LOWORD(nSum)+S_HIWORD(nSum));
}

XPE_DEF::IMAGE_RESOURCE_DIRECTORY_ENTRY XPE::read_IMAGE_RESOURCE_DIRECTORY_ENTRY(qint64 nOffset)
{
    XPE_DEF::IMAGE_RESOURCE_DIRECTORY_ENTRY result= {};

    read_array(nOffset,(char *)&result,sizeof(XPE_DEF::IMAGE_RESOURCE_DIRECTORY_ENTRY));

    return result;
}

XPE_DEF::IMAGE_RESOURCE_DIRECTORY XPE::read_IMAGE_RESOURCE_DIRECTORY(qint64 nOffset)
{
    XPE_DEF::IMAGE_RESOURCE_DIRECTORY result= {};

    read_array(nOffset,(char *)&result,sizeof(XPE_DEF::IMAGE_RESOURCE_DIRECTORY));

    return result;
}

XPE_DEF::IMAGE_RESOURCE_DATA_ENTRY XPE::read_IMAGE_RESOURCE_DATA_ENTRY(qint64 nOffset)
{
    XPE_DEF::IMAGE_RESOURCE_DATA_ENTRY result= {};

    read_array(nOffset,(char *)&result,sizeof(XPE_DEF::IMAGE_RESOURCE_DATA_ENTRY));

    return result;
}

XPE::RESOURCES_ID_NAME XPE::getResourcesIDName(qint64 nResourceOffset,quint32 value)
{
    RESOURCES_ID_NAME result= {};

    if(value&0x80000000)
    {
        result.bIsName=true;
        value&=0x7FFFFFFF;
        result.nNameOffset=value;
        result.nID=0;
        int nStringLength=read_uint16(nResourceOffset+value);
        QByteArray baName=read_array(nResourceOffset+value+2,nStringLength*2);
        result.sName=QString::fromUtf16((quint16 *)(baName.data()),nStringLength);
    }
    else
    {
        result.nID=value;
        result.sName="";
        result.nNameOffset=0;
    }

    return result;
}

QList<qint64> XPE::getRelocsAsRVAList()
{
    QSet<qint64> stResult;

    // TODO 64
    qint64 nRelocsOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_BASERELOC);

    if(nRelocsOffset!=-1)
    {
        while(true)
        {
            XPE_DEF::IMAGE_BASE_RELOCATION ibr= {};

            if(!read_array(nRelocsOffset,(char *)&ibr,sizeof(XPE_DEF::IMAGE_BASE_RELOCATION)))
            {
                break;
            }

            if((ibr.VirtualAddress==0)||(ibr.SizeOfBlock==0))
            {
                break;
            }

            nRelocsOffset+=sizeof(XPE_DEF::IMAGE_BASE_RELOCATION);

            int nCount=(ibr.SizeOfBlock-sizeof(XPE_DEF::IMAGE_BASE_RELOCATION))/sizeof(quint16);

            nCount=qMin(nCount,(int)0xFFFF);

            for(int i=0; i<nCount; i++)
            {
                quint16 nRecord=read_uint16(nRelocsOffset);

                if(nRecord)
                {
                    nRecord=nRecord&0x0FFF;
                    stResult.insert(ibr.VirtualAddress+nRecord);
                }

                nRelocsOffset+=sizeof(quint16);
            }
        }
    }

    return stResult.toList();
}

QList<XPE::RELOCS_HEADER> XPE::getRelocsHeaders()
{
    QList<XPE::RELOCS_HEADER> listResult;

    qint64 nRelocsOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_BASERELOC);

    if(nRelocsOffset!=-1)
    {
        while(true)
        {
            RELOCS_HEADER record= {0};

            record.nOffset=nRelocsOffset;

            if(!read_array(nRelocsOffset,(char *)&record.ibr,sizeof(XPE_DEF::IMAGE_BASE_RELOCATION)))
            {
                break;
            }

            if((record.ibr.VirtualAddress==0)||(record.ibr.SizeOfBlock==0))
            {
                break;
            }

            nRelocsOffset+=sizeof(XPE_DEF::IMAGE_BASE_RELOCATION);

            record.nCount=(record.ibr.SizeOfBlock-sizeof(XPE_DEF::IMAGE_BASE_RELOCATION))/sizeof(quint16);

            nRelocsOffset+=sizeof(quint16)*record.nCount;

            listResult.append(record);
        }
    }

    return listResult;
}

QList<XPE::RELOCS_POSITION> XPE::getRelocsPositions(qint64 nOffset)
{
    QList<XPE::RELOCS_POSITION> listResult;

    XPE_DEF::IMAGE_BASE_RELOCATION ibr= {0};

    if(read_array(nOffset,(char *)&ibr,sizeof(XPE_DEF::IMAGE_BASE_RELOCATION)))
    {
        if((ibr.VirtualAddress)&&(ibr.SizeOfBlock))
        {
            nOffset+=sizeof(XPE_DEF::IMAGE_BASE_RELOCATION);

            int nCount=(ibr.SizeOfBlock-sizeof(XPE_DEF::IMAGE_BASE_RELOCATION))/sizeof(quint16);

            nCount&=0xFFFF;

            for(int i=0; i<nCount; i++)
            {
                RELOCS_POSITION record= {0};

                quint16 nRecord=read_uint16(nOffset);

                record.nTypeOffset=nRecord;
                record.nAddress=ibr.VirtualAddress+(nRecord&0x0FFF);
                record.nType=nRecord>>12;

                listResult.append(record);

                nOffset+=sizeof(quint16);
            }
        }
    }

    return listResult;
}

bool XPE::addRelocsSection(QList<qint64> *pList)
{
    return addRelocsSection(getDevice(),isImage(),pList);
}

bool XPE::addRelocsSection(QIODevice *pDevice,bool bIsImage, QList<qint64> *pList)
{
    bool bResult=false;

    if((isResizeEnable(pDevice))&&(pList->count()))
    {
        XPE pe(pDevice,bIsImage);

        if(pe.isValid())
        {
            // Check valid
            QList<MEMORY_MAP> listMM=pe.getMemoryMapList();
            qint64 nBaseAddress=pe._getBaseAddress();
            QList<qint64> listRVAs;

            int _nCount=pList->count();

            for(int i=0; i<_nCount; i++)
            {
                if(pe.isAddressValid(&listMM,pList->at(i)+nBaseAddress))
                {
                    listRVAs.append(pList->at(i));
                }
            }

            QByteArray baRelocs=XPE::relocsAsRVAListToByteArray(&listRVAs,pe.is64());

            XPE_DEF::IMAGE_SECTION_HEADER ish= {};

            ish.Characteristics=0x42000040;
            QString sSectionName=".reloc";
            XBinary::_copyMemory((char *)&ish.Name,sSectionName.toLatin1().data(),qMin(XPE_DEF::S_IMAGE_SIZEOF_SHORT_NAME,sSectionName.length()));

            bResult=addSection(pDevice,bIsImage,&ish,baRelocs.data(),baRelocs.size());

            if(bResult)
            {
                XPE_DEF::IMAGE_DATA_DIRECTORY dd= {};

                dd.VirtualAddress=ish.VirtualAddress;
                dd.Size=ish.Misc.VirtualSize;

                pe.setOptionalHeader_DataDirectory(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_BASERELOC,&dd);

                bResult=true;
            }
        }
    }

    return bResult;
}

bool XPE::addRelocsSection(QString sFileName,bool bIsImage, QList<qint64> *pList)
{
    bool bResult=false;
    QFile file(sFileName);

    if(file.open(QIODevice::ReadWrite))
    {
        bResult=addRelocsSection(&file,bIsImage,pList);

        file.close();
    }

    return bResult;
}

QByteArray XPE::relocsAsRVAListToByteArray(QList<qint64> *pList, bool bIs64)
{
    QByteArray baResult;
    // GetHeaders

    // pList must be sorted!

    qint32 nBaseAddress=-1;
    quint32 nSize=0;

    for(int i=0; i<pList->count(); i++)
    {
        qint32 _nBaseAddress=S_ALIGN_DOWN(pList->at(i),0x1000);

        if(nBaseAddress!=_nBaseAddress)
        {
            nBaseAddress=_nBaseAddress;
            nSize=S_ALIGN_UP(nSize,4);
            nSize+=sizeof(XPE_DEF::IMAGE_BASE_RELOCATION);
        }

        nSize+=2;
    }

    nSize=S_ALIGN_UP(nSize,4);

    baResult.resize(nSize);

    nBaseAddress=-1;
    quint32 nOffset=0;
    char *pData=baResult.data();
    char *pVirtualAddress=0;
    char *pSizeOfBlock=0;
    quint32 nCurrentBlockSize=0;

    for(int i=0; i<pList->count(); i++)
    {
        qint32 _nBaseAddress=S_ALIGN_DOWN(pList->at(i),0x1000);

        if(nBaseAddress!=_nBaseAddress)
        {
            nBaseAddress=_nBaseAddress;
            quint32 _nOffset=S_ALIGN_UP(nOffset,4);

            if(nOffset!=_nOffset)
            {
                nCurrentBlockSize+=2;
                XBinary::_write_uint32(pSizeOfBlock,nCurrentBlockSize);
                XBinary::_write_uint16(pData+nOffset,0);
                nOffset=_nOffset;
            }

            pVirtualAddress=pData+nOffset+offsetof(XPE_DEF::IMAGE_BASE_RELOCATION,VirtualAddress);
            pSizeOfBlock=pData+nOffset+offsetof(XPE_DEF::IMAGE_BASE_RELOCATION,SizeOfBlock);
            XBinary::_write_uint32(pVirtualAddress,nBaseAddress);
            nCurrentBlockSize=sizeof(XPE_DEF::IMAGE_BASE_RELOCATION);
            XBinary::_write_uint32(pSizeOfBlock,nCurrentBlockSize);

            nOffset+=sizeof(XPE_DEF::IMAGE_BASE_RELOCATION);
        }

        nCurrentBlockSize+=2;
        XBinary::_write_uint32(pSizeOfBlock,nCurrentBlockSize);

        if(!bIs64)
        {
            XBinary::_write_uint16(pData+nOffset,pList->at(i)-nBaseAddress+0x3000);
        }
        else
        {
            XBinary::_write_uint16(pData+nOffset,pList->at(i)-nBaseAddress+0xA000);
        }

        nOffset+=2;
    }

    quint32 _nOffset=S_ALIGN_UP(nOffset,4);

    if(nOffset!=_nOffset)
    {
        nCurrentBlockSize+=2;
        XBinary::_write_uint32(pSizeOfBlock,nCurrentBlockSize);
        XBinary::_write_uint16(pData+nOffset,0);
    }

    return baResult;
}

bool XPE::isResourcesPresent()
{
    return isOptionalHeader_DataDirectoryPresent(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_RESOURCE);
}

bool XPE::isRelocsPresent()
{
    return isOptionalHeader_DataDirectoryPresent(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_BASERELOC);
}

bool XPE::isTLSPresent()
{
    return isOptionalHeader_DataDirectoryPresent(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_TLS);
}

bool XPE::isSignPresent()
{
    return isOptionalHeader_DataDirectoryPresent(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_SECURITY);
}

bool XPE::isExceptionPresent()
{
    return isOptionalHeader_DataDirectoryPresent(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_EXCEPTION);
}

XPE_DEF::S_IMAGE_TLS_DIRECTORY32 XPE::getTLSDirectory32()
{
    XPE_DEF::S_IMAGE_TLS_DIRECTORY32 result={};

    qint64 nTLSOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_TLS);

    if(nTLSOffset!=-1)
    {
        read_array(nTLSOffset,(char *)&result,sizeof(XPE_DEF::S_IMAGE_TLS_DIRECTORY32));
    }

    return result;
}

XPE_DEF::S_IMAGE_TLS_DIRECTORY64 XPE::getTLSDirectory64()
{
    XPE_DEF::S_IMAGE_TLS_DIRECTORY64 result={};

    qint64 nTLSOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_TLS);

    if(nTLSOffset!=-1)
    {
        read_array(nTLSOffset,(char *)&result,sizeof(XPE_DEF::S_IMAGE_TLS_DIRECTORY64));
    }

    return result;
}

quint64 XPE::getTLS_StartAddressOfRawData()
{
    quint64 nResult=0;

    qint64 nTLSOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_TLS);

    if(nTLSOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint64(nTLSOffset+offsetof(XPE_DEF::S_IMAGE_TLS_DIRECTORY64,StartAddressOfRawData));
        }
        else
        {
            nResult=read_uint32(nTLSOffset+offsetof(XPE_DEF::S_IMAGE_TLS_DIRECTORY32,StartAddressOfRawData));
        }
    }

    return nResult;
}

quint64 XPE::getTLS_EndAddressOfRawData()
{
    quint64 nResult=0;

    qint64 nTLSOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_TLS);

    if(nTLSOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint64(nTLSOffset+offsetof(XPE_DEF::S_IMAGE_TLS_DIRECTORY64,EndAddressOfRawData));
        }
        else
        {
            nResult=read_uint32(nTLSOffset+offsetof(XPE_DEF::S_IMAGE_TLS_DIRECTORY32,EndAddressOfRawData));
        }
    }

    return nResult;
}

quint64 XPE::getTLS_AddressOfIndex()
{
    quint64 nResult=0;

    qint64 nTLSOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_TLS);

    if(nTLSOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint64(nTLSOffset+offsetof(XPE_DEF::S_IMAGE_TLS_DIRECTORY64,AddressOfIndex));
        }
        else
        {
            nResult=read_uint32(nTLSOffset+offsetof(XPE_DEF::S_IMAGE_TLS_DIRECTORY32,AddressOfIndex));
        }
    }

    return nResult;
}

quint64 XPE::getTLS_AddressOfCallBacks()
{
    quint64 nResult=0;

    qint64 nTLSOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_TLS);

    if(nTLSOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint64(nTLSOffset+offsetof(XPE_DEF::S_IMAGE_TLS_DIRECTORY64,AddressOfCallBacks));
        }
        else
        {
            nResult=read_uint32(nTLSOffset+offsetof(XPE_DEF::S_IMAGE_TLS_DIRECTORY32,AddressOfCallBacks));
        }
    }

    return nResult;
}

quint32 XPE::getTLS_SizeOfZeroFill()
{
    quint32 nResult=0;

    qint64 nTLSOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_TLS);

    if(nTLSOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint64(nTLSOffset+offsetof(XPE_DEF::S_IMAGE_TLS_DIRECTORY64,SizeOfZeroFill));
        }
        else
        {
            nResult=read_uint32(nTLSOffset+offsetof(XPE_DEF::S_IMAGE_TLS_DIRECTORY32,SizeOfZeroFill));
        }
    }

    return nResult;
}

quint32 XPE::getTLS_Characteristics()
{
    quint32 nResult=0;

    qint64 nTLSOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_TLS);

    if(nTLSOffset!=-1)
    {
        if(is64())
        {
            nResult=read_uint64(nTLSOffset+offsetof(XPE_DEF::S_IMAGE_TLS_DIRECTORY64,Characteristics));
        }
        else
        {
            nResult=read_uint32(nTLSOffset+offsetof(XPE_DEF::S_IMAGE_TLS_DIRECTORY32,Characteristics));
        }
    }

    return nResult;
}

void XPE::setTLS_StartAddressOfRawData(quint64 value)
{
    qint64 nTLSOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_TLS);

    if(nTLSOffset!=-1)
    {
        if(is64())
        {
            write_uint64(nTLSOffset+offsetof(XPE_DEF::S_IMAGE_TLS_DIRECTORY64,StartAddressOfRawData),value);
        }
        else
        {
            write_uint32(nTLSOffset+offsetof(XPE_DEF::S_IMAGE_TLS_DIRECTORY32,StartAddressOfRawData),value);
        }
    }
}

void XPE::setTLS_EndAddressOfRawData(quint64 value)
{
    qint64 nTLSOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_TLS);

    if(nTLSOffset!=-1)
    {
        if(is64())
        {
            write_uint64(nTLSOffset+offsetof(XPE_DEF::S_IMAGE_TLS_DIRECTORY64,EndAddressOfRawData),value);
        }
        else
        {
            write_uint32(nTLSOffset+offsetof(XPE_DEF::S_IMAGE_TLS_DIRECTORY32,EndAddressOfRawData),value);
        }
    }
}

void XPE::setTLS_AddressOfIndex(quint64 value)
{
    qint64 nTLSOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_TLS);

    if(nTLSOffset!=-1)
    {
        if(is64())
        {
            write_uint64(nTLSOffset+offsetof(XPE_DEF::S_IMAGE_TLS_DIRECTORY64,AddressOfIndex),value);
        }
        else
        {
            write_uint32(nTLSOffset+offsetof(XPE_DEF::S_IMAGE_TLS_DIRECTORY32,AddressOfIndex),value);
        }
    }
}

void XPE::setTLS_AddressOfCallBacks(quint64 value)
{
    qint64 nTLSOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_TLS);

    if(nTLSOffset!=-1)
    {
        if(is64())
        {
            write_uint64(nTLSOffset+offsetof(XPE_DEF::S_IMAGE_TLS_DIRECTORY64,AddressOfCallBacks),value);
        }
        else
        {
            write_uint32(nTLSOffset+offsetof(XPE_DEF::S_IMAGE_TLS_DIRECTORY32,AddressOfCallBacks),value);
        }
    }
}

void XPE::setTLS_SizeOfZeroFill(quint32 value)
{
    qint64 nTLSOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_TLS);

    if(nTLSOffset!=-1)
    {
        if(is64())
        {
            write_uint64(nTLSOffset+offsetof(XPE_DEF::S_IMAGE_TLS_DIRECTORY64,SizeOfZeroFill),value);
        }
        else
        {
            write_uint32(nTLSOffset+offsetof(XPE_DEF::S_IMAGE_TLS_DIRECTORY32,SizeOfZeroFill),value);
        }
    }
}

void XPE::setTLS_Characteristics(quint32 value)
{
    qint64 nTLSOffset=getDataDirectoryOffset(XPE_DEF::S_IMAGE_DIRECTORY_ENTRY_TLS);

    if(nTLSOffset!=-1)
    {
        if(is64())
        {
            write_uint64(nTLSOffset+offsetof(XPE_DEF::S_IMAGE_TLS_DIRECTORY64,Characteristics),value);
        }
        else
        {
            write_uint32(nTLSOffset+offsetof(XPE_DEF::S_IMAGE_TLS_DIRECTORY32,Characteristics),value);
        }
    }
}

XPE::TLS_HEADER XPE::getTLSHeader()
{
    TLS_HEADER result= {};

    if(isTLSPresent())
    {
        if(is64())
        {
            XPE_DEF::S_IMAGE_TLS_DIRECTORY64 tls64=getTLSDirectory64();

            result.AddressOfCallBacks=tls64.AddressOfCallBacks;
            result.AddressOfIndex=tls64.AddressOfIndex;
            result.Characteristics=tls64.Characteristics;
            result.EndAddressOfRawData=tls64.EndAddressOfRawData;
            result.SizeOfZeroFill=tls64.SizeOfZeroFill;
            result.StartAddressOfRawData=tls64.StartAddressOfRawData;
        }
        else
        {
            XPE_DEF::S_IMAGE_TLS_DIRECTORY32 tls32=getTLSDirectory32();

            result.AddressOfCallBacks=tls32.AddressOfCallBacks;
            result.AddressOfIndex=tls32.AddressOfIndex;
            result.Characteristics=tls32.Characteristics;
            result.EndAddressOfRawData=tls32.EndAddressOfRawData;
            result.SizeOfZeroFill=tls32.SizeOfZeroFill;
            result.StartAddressOfRawData=tls32.StartAddressOfRawData;
        }
    }

    return result;
}

QMap<quint64, QString> XPE::getImageNtHeadersSignatures()
{
    QMap<quint64, QString> mapResult;

    mapResult.insert(0x00004550,"IMAGE_NT_SIGNATURE");

    return mapResult;
}

QMap<quint64, QString> XPE::getImageNtHeadersSignaturesS()
{
    QMap<quint64, QString> mapResult;

    mapResult.insert(0x00004550,"NT_SIGNATURE");

    return mapResult;
}

QMap<quint64, QString> XPE::getImageMagics()
{
    QMap<quint64, QString> mapResult;

    mapResult.insert(0x5A4D,"IMAGE_DOS_SIGNATURE");

    return mapResult;
}

QMap<quint64, QString> XPE::getImageMagicsS()
{
    QMap<quint64, QString> mapResult;

    mapResult.insert(0x5A4D,"DOS_SIGNATURE");

    return mapResult;
}

QMap<quint64, QString> XPE::getImageFileHeaderMachines()
{
    QMap<quint64, QString> mapResult;
    mapResult.insert(0,"IMAGE_FILE_MACHINE_UNKNOWN");
    mapResult.insert(0x014c,"IMAGE_FILE_MACHINE_I386");
    mapResult.insert(0x0162,"IMAGE_FILE_MACHINE_R3000");
    mapResult.insert(0x0166,"IMAGE_FILE_MACHINE_R4000");
    mapResult.insert(0x0168,"IMAGE_FILE_MACHINE_R10000");
    mapResult.insert(0x0169,"IMAGE_FILE_MACHINE_WCEMIPSV2");
    mapResult.insert(0x0184,"IMAGE_FILE_MACHINE_ALPHA");
    mapResult.insert(0x01a2,"IMAGE_FILE_MACHINE_SH3");
    mapResult.insert(0x01a3,"IMAGE_FILE_MACHINE_SH3DSP");
    mapResult.insert(0x01a4,"IMAGE_FILE_MACHINE_SH3E");
    mapResult.insert(0x01a6,"IMAGE_FILE_MACHINE_SH4");
    mapResult.insert(0x01a8,"IMAGE_FILE_MACHINE_SH5");
    mapResult.insert(0x01c0,"IMAGE_FILE_MACHINE_ARM");
    mapResult.insert(0x01c2,"IMAGE_FILE_MACHINE_THUMB");
    mapResult.insert(0x01c4,"IMAGE_FILE_MACHINE_ARMNT");
    mapResult.insert(0x01d3,"IMAGE_FILE_MACHINE_AM33");
    mapResult.insert(0x01F0,"IMAGE_FILE_MACHINE_POWERPC");
    mapResult.insert(0x01f1,"IMAGE_FILE_MACHINE_POWERPCFP");
    mapResult.insert(0x0200,"IMAGE_FILE_MACHINE_IA64");
    mapResult.insert(0x0266,"IMAGE_FILE_MACHINE_MIPS16");
    mapResult.insert(0x0284,"IMAGE_FILE_MACHINE_ALPHA64");
    mapResult.insert(0x0366,"IMAGE_FILE_MACHINE_MIPSFPU");
    mapResult.insert(0x0466,"IMAGE_FILE_MACHINE_MIPSFPU16");
    mapResult.insert(0x0520,"IMAGE_FILE_MACHINE_TRICORE");
    mapResult.insert(0x0CEF,"IMAGE_FILE_MACHINE_CEF");
    mapResult.insert(0x0EBC,"IMAGE_FILE_MACHINE_EBC");
    mapResult.insert(0x8664,"IMAGE_FILE_MACHINE_AMD64");
    mapResult.insert(0x9041,"IMAGE_FILE_MACHINE_M32R");
    mapResult.insert(0xC0EE,"IMAGE_FILE_MACHINE_CEE");
    mapResult.insert(0xAA64,"IMAGE_FILE_MACHINE_ARM64");

    return mapResult;
}

QMap<quint64, QString> XPE::getImageFileHeaderMachinesS()
{
    QMap<quint64, QString> mapResult;
    mapResult.insert(0,"UNKNOWN");
    mapResult.insert(0x014c,"I386");
    mapResult.insert(0x0162,"R3000");
    mapResult.insert(0x0166,"R4000");
    mapResult.insert(0x0168,"R10000");
    mapResult.insert(0x0169,"WCEMIPSV2");
    mapResult.insert(0x0184,"ALPHA");
    mapResult.insert(0x01a2,"SH3");
    mapResult.insert(0x01a3,"SH3DSP");
    mapResult.insert(0x01a4,"SH3E");
    mapResult.insert(0x01a6,"SH4");
    mapResult.insert(0x01a8,"SH5");
    mapResult.insert(0x01c0,"ARM");
    mapResult.insert(0x01c2,"THUMB");
    mapResult.insert(0x01c4,"ARMNT");
    mapResult.insert(0x01d3,"AM33");
    mapResult.insert(0x01F0,"POWERPC");
    mapResult.insert(0x01f1,"POWERPCFP");
    mapResult.insert(0x0200,"IA64");
    mapResult.insert(0x0266,"MIPS16");
    mapResult.insert(0x0284,"ALPHA64");
    mapResult.insert(0x0366,"MIPSFPU");
    mapResult.insert(0x0466,"MIPSFPU16");
    mapResult.insert(0x0520,"TRICORE");
    mapResult.insert(0x0CEF,"CEF");
    mapResult.insert(0x0EBC,"EBC");
    mapResult.insert(0x8664,"AMD64");
    mapResult.insert(0x9041,"M32R");
    mapResult.insert(0xC0EE,"CEE");
    mapResult.insert(0xAA64,"ARM64");
    return mapResult;
}

QMap<quint64, QString> XPE::getImageFileHeaderCharacteristics()
{
    QMap<quint64, QString> mapResult;
    mapResult.insert(0x0001,"IMAGE_FILE_RELOCS_STRIPPED");
    mapResult.insert(0x0002,"IMAGE_FILE_EXECUTABLE_IMAGE");
    mapResult.insert(0x0004,"IMAGE_FILE_LINE_NUMS_STRIPPED");
    mapResult.insert(0x0008,"IMAGE_FILE_LOCAL_SYMS_STRIPPED");
    mapResult.insert(0x0010,"IMAGE_FILE_AGGRESIVE_WS_TRIM");
    mapResult.insert(0x0020,"IMAGE_FILE_LARGE_ADDRESS_AWARE");
    mapResult.insert(0x0080,"IMAGE_FILE_BYTES_REVERSED_LO");
    mapResult.insert(0x0100,"IMAGE_FILE_32BIT_MACHINE");
    mapResult.insert(0x0200,"IMAGE_FILE_DEBUG_STRIPPED");
    mapResult.insert(0x0400,"IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP");
    mapResult.insert(0x0800,"IMAGE_FILE_NET_RUN_FROM_SWAP");
    mapResult.insert(0x1000,"IMAGE_FILE_SYSTEM");
    mapResult.insert(0x2000,"IMAGE_FILE_DLL");
    mapResult.insert(0x4000,"IMAGE_FILE_UP_SYSTEM_ONLY");
    mapResult.insert(0x8000,"IMAGE_FILE_BYTES_REVERSED_HI");
    return mapResult;
}

QMap<quint64, QString> XPE::getImageFileHeaderCharacteristicsS()
{
    QMap<quint64, QString> mapResult;
    mapResult.insert(0x0001,"RELOCS_STRIPPED");
    mapResult.insert(0x0002,"EXECUTABLE_IMAGE");
    mapResult.insert(0x0004,"LINE_NUMS_STRIPPED");
    mapResult.insert(0x0008,"LOCAL_SYMS_STRIPPED");
    mapResult.insert(0x0010,"AGGRESIVE_WS_TRIM");
    mapResult.insert(0x0020,"LARGE_ADDRESS_AWARE");
    mapResult.insert(0x0080,"BYTES_REVERSED_LO");
    mapResult.insert(0x0100,"32BIT_MACHINE");
    mapResult.insert(0x0200,"DEBUG_STRIPPED");
    mapResult.insert(0x0400,"REMOVABLE_RUN_FROM_SWAP");
    mapResult.insert(0x0800,"NET_RUN_FROM_SWAP");
    mapResult.insert(0x1000,"SYSTEM");
    mapResult.insert(0x2000,"DLL");
    mapResult.insert(0x4000,"UP_SYSTEM_ONLY");
    mapResult.insert(0x8000,"BYTES_REVERSED_HI");
    return mapResult;
}

QMap<quint64, QString> XPE::getImageOptionalHeaderMagic()
{
    QMap<quint64, QString> mapResult;
    mapResult.insert(0x10b,"IMAGE_NT_OPTIONAL_HDR32_MAGIC");
    mapResult.insert(0x20b,"IMAGE_NT_OPTIONAL_HDR64_MAGIC");
    mapResult.insert(0x107,"IMAGE_ROM_OPTIONAL_HDR_MAGIC");
    return mapResult;
}

QMap<quint64, QString> XPE::getImageOptionalHeaderMagicS()
{
    QMap<quint64, QString> mapResult;
    mapResult.insert(0x10b,"NT_HDR32_MAGIC");
    mapResult.insert(0x20b,"NT_HDR64_MAGIC");
    mapResult.insert(0x107,"ROM_HDR_MAGIC");
    return mapResult;
}

QMap<quint64, QString> XPE::getImageOptionalHeaderSubsystem()
{
    QMap<quint64, QString> mapResult;
    mapResult.insert(0,"IMAGE_SUBSYSTEM_UNKNOWN");
    mapResult.insert(1,"IMAGE_SUBSYSTEM_NATIVE");
    mapResult.insert(2,"IMAGE_SUBSYSTEM_WINDOWS_GUI");
    mapResult.insert(3,"IMAGE_SUBSYSTEM_WINDOWS_CUI");
    mapResult.insert(5,"IMAGE_SUBSYSTEM_OS2_CUI");
    mapResult.insert(7,"IMAGE_SUBSYSTEM_POSIX_CUI");
    mapResult.insert(8,"IMAGE_SUBSYSTEM_NATIVE_WINDOWS");
    mapResult.insert(9,"IMAGE_SUBSYSTEM_WINDOWS_CE_GUI");
    mapResult.insert(10,"IMAGE_SUBSYSTEM_EFI_APPLICATION");
    mapResult.insert(11,"IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER");
    mapResult.insert(12,"IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER");
    mapResult.insert(13,"IMAGE_SUBSYSTEM_EFI_ROM");
    mapResult.insert(14,"IMAGE_SUBSYSTEM_XBOX");
    mapResult.insert(16,"IMAGE_SUBSYSTEM_WINDOWS_BOOT_APPLICATION");
    return mapResult;
}

QMap<quint64, QString> XPE::getImageOptionalHeaderSubsystemS()
{
    QMap<quint64, QString> mapResult;
    mapResult.insert(0,"UNKNOWN");
    mapResult.insert(1,"NATIVE");
    mapResult.insert(2,"WINDOWS_GUI");
    mapResult.insert(3,"WINDOWS_CUI");
    mapResult.insert(5,"OS2_CUI");
    mapResult.insert(7,"POSIX_CUI");
    mapResult.insert(8,"NATIVE_WINDOWS");
    mapResult.insert(9,"WINDOWS_CE_GUI");
    mapResult.insert(10,"EFI_APPLICATION");
    mapResult.insert(11,"EFI_BOOT_SERVICE_DRIVER");
    mapResult.insert(12,"EFI_RUNTIME_DRIVER");
    mapResult.insert(13,"EFI_ROM");
    mapResult.insert(14,"XBOX");
    mapResult.insert(16,"WINDOWS_BOOT_APPLICATION");
    return mapResult;
}

QMap<quint64, QString> XPE::getImageOptionalHeaderDllCharacteristics()
{
    QMap<quint64, QString> mapResult;
    mapResult.insert(0x0020,"IMAGE_DLLCHARACTERISTICS_HIGH_ENTROPY_VA");
    mapResult.insert(0x0040,"IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE");
    mapResult.insert(0x0080,"IMAGE_DLLCHARACTERISTICS_FORCE_INTEGRITY");
    mapResult.insert(0x0100,"IMAGE_DLLCHARACTERISTICS_NX_COMPAT");
    mapResult.insert(0x0200,"IMAGE_DLLCHARACTERISTICS_NO_ISOLATION");
    mapResult.insert(0x0400,"IMAGE_DLLCHARACTERISTICS_NO_SEH");
    mapResult.insert(0x0800,"IMAGE_DLLCHARACTERISTICS_NO_BIND");
    mapResult.insert(0x1000,"IMAGE_DLLCHARACTERISTICS_APPCONTAINER");
    mapResult.insert(0x2000,"IMAGE_DLLCHARACTERISTICS_WDM_DRIVER");
    mapResult.insert(0x4000,"IMAGE_DLLCHARACTERISTICS_GUARD_CF");
    mapResult.insert(0x8000,"IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE");
    return mapResult;
}

QMap<quint64, QString> XPE::getImageOptionalHeaderDllCharacteristicsS()
{
    QMap<quint64, QString> mapResult;
    mapResult.insert(0x0020,"HIGH_ENTROPY_VA");
    mapResult.insert(0x0040,"DYNAMIC_BASE");
    mapResult.insert(0x0080,"FORCE_INTEGRITY");
    mapResult.insert(0x0100,"NX_COMPAT");
    mapResult.insert(0x0200,"NO_ISOLATION");
    mapResult.insert(0x0400,"NO_SEH");
    mapResult.insert(0x0800,"NO_BIND");
    mapResult.insert(0x1000,"APPCONTAINER");
    mapResult.insert(0x2000,"WDM_DRIVER");
    mapResult.insert(0x4000,"GUARD_CF");
    mapResult.insert(0x8000,"TERMINAL_SERVER_AWARE");
    return mapResult;
}

QMap<quint64, QString> XPE::getImageSectionHeaderFlags()
{
    QMap<quint64, QString> mapResult;
    mapResult.insert(0x00000008,"IMAGE_SCN_TYPE_NO_PAD");
    mapResult.insert(0x00000020,"IMAGE_SCN_CNT_CODE");
    mapResult.insert(0x00000040,"IMAGE_SCN_CNT_INITIALIZED_DATA");
    mapResult.insert(0x00000080,"IMAGE_SCN_CNT_UNINITIALIZED_DATA");
    mapResult.insert(0x00000100,"IMAGE_SCN_LNK_OTHER");
    mapResult.insert(0x00000200,"IMAGE_SCN_LNK_INFO");
    mapResult.insert(0x00000800,"IMAGE_SCN_LNK_REMOVE");
    mapResult.insert(0x00001000,"IMAGE_SCN_LNK_COMDAT");
    mapResult.insert(0x00004000,"IMAGE_SCN_NO_DEFER_SPEC_EXC");
    mapResult.insert(0x00008000,"IMAGE_SCN_GPREL");
    mapResult.insert(0x00020000,"IMAGE_SCN_MEM_PURGEABLE");
    mapResult.insert(0x00020000,"IMAGE_SCN_MEM_16BIT");
    mapResult.insert(0x00040000,"IMAGE_SCN_MEM_LOCKED");
    mapResult.insert(0x00080000,"IMAGE_SCN_MEM_PRELOAD");
    mapResult.insert(0x01000000,"IMAGE_SCN_LNK_NRELOC_OVFL");
    mapResult.insert(0x02000000,"IMAGE_SCN_MEM_DISCARDABLE");
    mapResult.insert(0x04000000,"IMAGE_SCN_MEM_NOT_CACHED");
    mapResult.insert(0x08000000,"IMAGE_SCN_MEM_NOT_PAGED");
    mapResult.insert(0x10000000,"IMAGE_SCN_MEM_SHARED");
    mapResult.insert(0x20000000,"IMAGE_SCN_MEM_EXECUTE");
    mapResult.insert(0x40000000,"IMAGE_SCN_MEM_READ");
    mapResult.insert(0x80000000,"IMAGE_SCN_MEM_WRITE");
    return mapResult;
}

QMap<quint64, QString> XPE::getImageSectionHeaderFlagsS()
{
    QMap<quint64, QString> mapResult;
    mapResult.insert(0x00000008,"TYPE_NO_PAD");
    mapResult.insert(0x00000020,"CNT_CODE");
    mapResult.insert(0x00000040,"CNT_INITIALIZED_DATA");
    mapResult.insert(0x00000080,"CNT_UNINITIALIZED_DATA");
    mapResult.insert(0x00000100,"LNK_OTHER");
    mapResult.insert(0x00000200,"LNK_INFO");
    mapResult.insert(0x00000800,"LNK_REMOVE");
    mapResult.insert(0x00001000,"LNK_COMDAT");
    mapResult.insert(0x00004000,"NO_DEFER_SPEC_EXC");
    mapResult.insert(0x00008000,"GPREL");
    mapResult.insert(0x00020000,"MEM_PURGEABLE");
    mapResult.insert(0x00020000,"MEM_16BIT");
    mapResult.insert(0x00040000,"MEM_LOCKED");
    mapResult.insert(0x00080000,"MEM_PRELOAD");
    mapResult.insert(0x01000000,"LNK_NRELOC_OVFL");
    mapResult.insert(0x02000000,"MEM_DISCARDABLE");
    mapResult.insert(0x04000000,"MEM_NOT_CACHED");
    mapResult.insert(0x08000000,"MEM_NOT_PAGED");
    mapResult.insert(0x10000000,"MEM_SHARED");
    mapResult.insert(0x20000000,"MEM_EXECUTE");
    mapResult.insert(0x40000000,"MEM_READ");
    mapResult.insert(0x80000000,"MEM_WRITE");
    return mapResult;
}

QMap<quint64, QString> XPE::getImageSectionHeaderAligns()
{
    QMap<quint64, QString> mapResult;
    mapResult.insert(0x00100000,"IMAGE_SCN_ALIGN_1BYTES");
    mapResult.insert(0x00200000,"IMAGE_SCN_ALIGN_2BYTES");
    mapResult.insert(0x00300000,"IMAGE_SCN_ALIGN_4BYTES");
    mapResult.insert(0x00400000,"IMAGE_SCN_ALIGN_8BYTES");
    mapResult.insert(0x00500000,"IMAGE_SCN_ALIGN_16BYTES");
    mapResult.insert(0x00600000,"IMAGE_SCN_ALIGN_32BYTES");
    mapResult.insert(0x00700000,"IMAGE_SCN_ALIGN_64BYTES");
    mapResult.insert(0x00800000,"IMAGE_SCN_ALIGN_128BYTES");
    mapResult.insert(0x00900000,"IMAGE_SCN_ALIGN_256BYTES");
    mapResult.insert(0x00A00000,"IMAGE_SCN_ALIGN_512BYTES");
    mapResult.insert(0x00B00000,"IMAGE_SCN_ALIGN_1024BYTES");
    mapResult.insert(0x00C00000,"IMAGE_SCN_ALIGN_2048BYTES");
    mapResult.insert(0x00D00000,"IMAGE_SCN_ALIGN_4096BYTES");
    mapResult.insert(0x00E00000,"IMAGE_SCN_ALIGN_8192BYTES");
    return mapResult;
}

QMap<quint64, QString> XPE::getImageSectionHeaderAlignsS()
{
    QMap<quint64, QString> mapResult;
    mapResult.insert(0x00100000,"1BYTES");
    mapResult.insert(0x00200000,"2BYTES");
    mapResult.insert(0x00300000,"4BYTES");
    mapResult.insert(0x00400000,"8BYTES");
    mapResult.insert(0x00500000,"16BYTES");
    mapResult.insert(0x00600000,"32BYTES");
    mapResult.insert(0x00700000,"64BYTES");
    mapResult.insert(0x00800000,"128BYTES");
    mapResult.insert(0x00900000,"256BYTES");
    mapResult.insert(0x00A00000,"512BYTES");
    mapResult.insert(0x00B00000,"1024BYTES");
    mapResult.insert(0x00C00000,"2048BYTES");
    mapResult.insert(0x00D00000,"4096BYTES");
    mapResult.insert(0x00E00000,"8192BYTES");
    return mapResult;
}

QMap<quint64, QString> XPE::getResourceTypes()
{
    QMap<quint64, QString> mapResult;
    mapResult.insert(1,"RT_CURSOR");
    mapResult.insert(2,"RT_BITMAP");
    mapResult.insert(3,"RT_ICON");
    mapResult.insert(4,"RT_MENU");
    mapResult.insert(5,"RT_DIALOG");
    mapResult.insert(6,"RT_STRING");
    mapResult.insert(7,"RT_FONTDIR");
    mapResult.insert(8,"RT_FONT");
    mapResult.insert(9,"RT_ACCELERATORS");
    mapResult.insert(10,"RT_RCDATA");
    mapResult.insert(11,"RT_MESSAGETABLE");
    mapResult.insert(12,"RT_GROUP_CURSOR");
    mapResult.insert(14,"RT_GROUP_ICON");
    mapResult.insert(16,"RT_VERSION");
    mapResult.insert(24,"RT_MANIFEST");
    mapResult.insert(0x2000+2,"RT_NEWBITMAP");
    mapResult.insert(0x2000+4,"RT_NEWMENU");
    mapResult.insert(0x2000+5,"RT_NEWDIALOG");
    return mapResult;
}

QMap<quint64, QString> XPE::getResourceTypesS()
{
    QMap<quint64, QString> mapResult;
    mapResult.insert(1,"CURSOR");
    mapResult.insert(2,"BITMAP");
    mapResult.insert(3,"ICON");
    mapResult.insert(4,"MENU");
    mapResult.insert(5,"DIALOG");
    mapResult.insert(6,"STRING");
    mapResult.insert(7,"FONTDIR");
    mapResult.insert(8,"FONT");
    mapResult.insert(9,"ACCELERATORS");
    mapResult.insert(10,"RCDATA");
    mapResult.insert(11,"MESSAGETABLE");
    mapResult.insert(12,"GROUP_CURSOR");
    mapResult.insert(14,"GROUP_ICON");
    mapResult.insert(16,"VERSION");
    mapResult.insert(24,"MANIFEST");
    mapResult.insert(0x2000+2,"NEWBITMAP");
    mapResult.insert(0x2000+4,"NEWMENU");
    mapResult.insert(0x2000+5,"NEWDIALOG");
    return mapResult;
}

QMap<quint64, QString> XPE::getImageRelBased()
{
    QMap<quint64, QString> mapResult;
    mapResult.insert(0,"IMAGE_REL_BASED_ABSOLUTE");
    mapResult.insert(1,"IMAGE_REL_BASED_HIGH");
    mapResult.insert(2,"IMAGE_REL_BASED_LOW");
    mapResult.insert(3,"IMAGE_REL_BASED_HIGHLOW");
    mapResult.insert(4,"IMAGE_REL_BASED_HIGHADJ");
    mapResult.insert(5,"IMAGE_REL_BASED_MACHINE_SPECIFIC_5");
    mapResult.insert(6,"IMAGE_REL_BASED_RESERVED");
    mapResult.insert(7,"IMAGE_REL_BASED_MACHINE_SPECIFIC_7");
    mapResult.insert(8,"IMAGE_REL_BASED_MACHINE_SPECIFIC_8");
    mapResult.insert(9,"IMAGE_REL_BASED_MACHINE_SPECIFIC_9");
    mapResult.insert(10,"IMAGE_REL_BASED_DIR64");
    return mapResult;
}

QMap<quint64, QString> XPE::getImageRelBasedS()
{
    QMap<quint64, QString> mapResult;
    mapResult.insert(0,"ABSOLUTE");
    mapResult.insert(1,"HIGH");
    mapResult.insert(2,"LOW");
    mapResult.insert(3,"HIGHLOW");
    mapResult.insert(4,"HIGHADJ");
    mapResult.insert(5,"MACHINE_SPECIFIC_5");
    mapResult.insert(6,"RESERVED");
    mapResult.insert(7,"MACHINE_SPECIFIC_7");
    mapResult.insert(8,"MACHINE_SPECIFIC_8");
    mapResult.insert(9,"MACHINE_SPECIFIC_9");
    mapResult.insert(10,"DIR64");
    return mapResult;
}
