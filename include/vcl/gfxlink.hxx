/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */

#ifndef INCLUDED_VCL_GFXLINK_HXX
#define INCLUDED_VCL_GFXLINK_HXX

#include <rtl/ustring.hxx>
#include <tools/gen.hxx>
#include <tools/solar.h>
#include <vcl/dllapi.h>
#include <vcl/mapmod.hxx>

class SvStream;


struct ImpBuffer
{
    sal_uLong       mnRefCount;
    sal_uInt8*      mpBuffer;

                ImpBuffer( sal_uLong nSize )
                {
                    mnRefCount = 1UL;
                    mpBuffer = nSize ? new sal_uInt8[ nSize ] : nullptr;
                }

                ImpBuffer( sal_uInt8* pBuf ) { mnRefCount = 1UL; mpBuffer = pBuf; }

                ~ImpBuffer() { delete[] mpBuffer; }
};


struct ImpSwap
{
    OUString   maURL;
    sal_uLong           mnDataSize;
    sal_uLong           mnRefCount;

                    ImpSwap( sal_uInt8* pData, sal_uLong nDataSize );
                    ~ImpSwap();

    sal_uInt8*          GetData() const;

    bool            IsSwapped() const { return maURL.getLength() > 0; }

    void            WriteTo( SvStream& rOStm ) const;
};


struct ImpGfxLink
{
    MapMode         maPrefMapMode;
    Size            maPrefSize;
    bool            mbPrefMapModeValid;
    bool            mbPrefSizeValid;

    ImpGfxLink() :
        maPrefMapMode(),
        maPrefSize(),
        mbPrefMapModeValid( false ),
        mbPrefSizeValid( false )
    {}
};

//#endif // __PRIVATE


enum class GfxLinkType
{
    None          = 0,
    EPS_Buffer    = 1,
    Native_GIF    = 2,
    Native_JPEG   = 3,
    Native_PNG    = 4,
    Native_TIFF   = 5,
    Native_WMF    = 6,
    Native_MET    = 7,
    Native_PICT   = 8,
    Native_SVG    = 9,
    Native_MOV    = 10,
    Native_BMP    = 11,  // #i15508# added support for BMP type
    User          = 0xffff
};

#define GFX_LINK_FIRST_NATIVE_ID    GfxLinkType::Native_GIF
#define GFX_LINK_LAST_NATIVE_ID     GfxLinkType::Native_BMP


struct ImpBuffer;
struct ImpSwap;
struct ImpGfxLink;
class Graphic;

class VCL_DLLPUBLIC GfxLink
{
private:

    GfxLinkType         myType;
    ImpBuffer*          mpBuf;
    ImpSwap*            mpSwap;
    sal_uInt32          mnBufSize;
    sal_uInt32          mnUserId;
    ImpGfxLink*         mpImpData;

    SAL_DLLPRIVATE void ImplCopy( const GfxLink& rGfxLink );

public:
                        GfxLink();
                        GfxLink( const GfxLink& );
                        GfxLink( const OUString& rPath, GfxLinkType nType );
                        GfxLink( sal_uInt8* pBuf, sal_uInt32 nBufSize, GfxLinkType nType );
                        ~GfxLink();

    GfxLink&            operator=( const GfxLink& );
    bool            IsEqual( const GfxLink& ) const;

    GfxLinkType         GetType() const { return myType; }

    void                SetUserId( sal_uInt32 nUserId ) { mnUserId = nUserId; }
    sal_uInt32          GetUserId() const { return mnUserId; }

    sal_uInt32          GetDataSize() const { return mnBufSize;}
    const sal_uInt8*    GetData() const;

    const Size&         GetPrefSize() const { return mpImpData->maPrefSize;}
    void                SetPrefSize( const Size& rPrefSize );
    bool                IsPrefSizeValid() { return mpImpData->mbPrefSizeValid;}

    const MapMode&      GetPrefMapMode() const { return mpImpData->maPrefMapMode;}
    void                SetPrefMapMode( const MapMode& rPrefMapMode );
    bool                IsPrefMapModeValid() { return mpImpData->mbPrefMapModeValid;}

    bool                IsNative() const;

    bool                LoadNative( Graphic& rGraphic );

    bool                ExportNative( SvStream& rOStream ) const;

    void                SwapOut();
    void                SwapIn();
    bool                IsSwappedOut() const { return( mpSwap != nullptr ); }

public:

    friend VCL_DLLPUBLIC SvStream&  WriteGfxLink( SvStream& rOStream, const GfxLink& rGfxLink );
    friend VCL_DLLPUBLIC SvStream&  ReadGfxLink( SvStream& rIStream, GfxLink& rGfxLink );
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
