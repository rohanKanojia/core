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

#include <config_gpgme.h>
#include <certificatechooser.hxx>
#include <certificateviewer.hxx>
#include <biginteger.hxx>
#include <com/sun/star/xml/crypto/XSecurityEnvironment.hpp>
#include <comphelper/sequence.hxx>
#include <comphelper/xmlsechelper.hxx>

#include <com/sun/star/security/NoPasswordException.hpp>
#include <com/sun/star/security/CertificateCharacters.hpp>

#include <vcl/treelistentry.hxx>
#include <unotools/datetime.hxx>
#include <unotools/useroptions.hxx>

using namespace comphelper;
using namespace css;

CertificateChooser::CertificateChooser(vcl::Window* _pParent,
                                       uno::Reference<uno::XComponentContext> const & _rxCtx,
                                       std::vector< css::uno::Reference< css::xml::crypto::XXMLSecurityContext > > const & rxSecurityContexts,
                                       UserAction eAction)
    : ModalDialog(_pParent, "SelectCertificateDialog", "xmlsec/ui/selectcertificatedialog.ui"),
    mvUserData(),
    meAction( eAction )
{
    get(m_pFTSign, "sign");
    get(m_pFTEncrypt, "encrypt");
    get(m_pOKBtn, "ok");
    get(m_pViewBtn, "viewcert");
    get(m_pFTDescription, "description-label");
    get(m_pDescriptionED, "description");

    Size aControlSize(475, 122);
    const long nControlWidth = aControlSize.Width();
    aControlSize = LogicToPixel(aControlSize, MapMode(MapUnit::MapAppFont));
    SvSimpleTableContainer *pSignatures = get<SvSimpleTableContainer>("signatures");
    pSignatures->set_width_request(aControlSize.Width());
    pSignatures->set_height_request(aControlSize.Height());

    m_pCertLB = VclPtr<SvSimpleTable>::Create(*pSignatures);
    static long nTabs[] = { 0, 20*nControlWidth/100, 50*nControlWidth/100, 60*nControlWidth/100, 70*nControlWidth/100  };
    m_pCertLB->SetTabs( SAL_N_ELEMENTS(nTabs), nTabs );
    m_pCertLB->InsertHeaderEntry(get<FixedText>("issuedto")->GetText() + "\t" + get<FixedText>("issuedby")->GetText()
        + "\t" + get<FixedText>("type")->GetText() + "\t" + get<FixedText>("expiration")->GetText()
        + "\t" + get<FixedText>("usage")->GetText());
    m_pCertLB->SetSelectHdl( LINK( this, CertificateChooser, CertificateHighlightHdl ) );
    m_pCertLB->SetDoubleClickHdl( LINK( this, CertificateChooser, CertificateSelectHdl ) );
    m_pViewBtn->SetClickHdl( LINK( this, CertificateChooser, ViewButtonHdl ) );

    mxCtx = _rxCtx;
    mxSecurityContexts = rxSecurityContexts;
    mbInitialized = false;

    // disable buttons
    CertificateHighlightHdl( nullptr );
}

CertificateChooser::~CertificateChooser()
{
    disposeOnce();
}

void CertificateChooser::dispose()
{
    m_pFTSign.clear();
    m_pFTEncrypt.clear();
    m_pCertLB.disposeAndClear();
    m_pViewBtn.clear();
    m_pOKBtn.clear();
    m_pFTDescription.clear();
    m_pDescriptionED.clear();
    mvUserData.clear();
    ModalDialog::dispose();
}

short CertificateChooser::Execute()
{
    // #i48432#
    // We can't check for personal certificates before raising this dialog,
    // because the mozilla implementation throws a NoPassword exception,
    // if the user pressed cancel, and also if the database does not exist!
    // But in the later case, the is no password query, and the user is confused
    // that nothing happens when pressing "Add..." in the SignatureDialog.

    // PostUserEvent( LINK( this, CertificateChooser, Initialize ) );

    // PostUserLink behavior is to slow, so do it directly before Execute().
    // Problem: This Dialog should be visible right now, and the parent should not be accessible.
    // Show, Update, DisableInput...

    vcl::Window* pMe = this;
    vcl::Window* pParent = GetParent();
    if ( pParent )
        pParent->EnableInput( false );
    pMe->Show();
    pMe->Update();
    ImplInitialize();
    if ( pParent )
        pParent->EnableInput();
    return ModalDialog::Execute();
}

void CertificateChooser::HandleOneUsageBit(OUString& string, int& bits, int bit, const char *name)
{
    if (bits & bit)
    {
        if (!string.isEmpty())
            string += ", ";
        string += get<FixedText>(OString("STR_") + name)->GetText();
        bits &= ~bit;
    }
}

OUString CertificateChooser::UsageInClearText(int bits)
{
    OUString result;

    HandleOneUsageBit(result, bits, 0x80, "DIGITAL_SIGNATURE");
    HandleOneUsageBit(result, bits, 0x40, "NON_REPUDIATION");
    HandleOneUsageBit(result, bits, 0x20, "KEY_ENCIPHERMENT");
    HandleOneUsageBit(result, bits, 0x10, "DATA_ENCIPHERMENT");
    HandleOneUsageBit(result, bits, 0x08, "KEY_AGREEMENT");
    HandleOneUsageBit(result, bits, 0x04, "KEY_CERT_SIGN");
    HandleOneUsageBit(result, bits, 0x02, "CRL_SIGN");
    HandleOneUsageBit(result, bits, 0x01, "ENCIPHER_ONLY");

    // Check for mystery leftover bits
    if (bits != 0)
    {
        if (!result.isEmpty())
            result += ", ";
        result += "0x" + OUString::number(bits, 16);
    }

    return result;
}

void CertificateChooser::ImplInitialize()
{
    if ( mbInitialized )
        return;

    SvtUserOptions aUserOpts;

    switch (meAction)
    {
        case UserAction::Sign:
            m_pFTSign->Show();
            m_pOKBtn->SetText( get<FixedText>("str_sign")->GetText() );
            msPreferredKey = aUserOpts.GetSigningKey();
            break;

        case UserAction::SelectSign:
            m_pFTSign->Show();
            m_pOKBtn->SetText( get<FixedText>("str_selectsign")->GetText() );
            msPreferredKey = aUserOpts.GetSigningKey();
            break;

        case UserAction::Encrypt:
            m_pFTEncrypt->Show();
            m_pFTDescription->Hide();
            m_pDescriptionED->Hide();
            m_pCertLB->SetSelectionMode( SelectionMode::Multiple );
            m_pOKBtn->SetText( get<FixedText>("str_encrypt")->GetText() );
            msPreferredKey = aUserOpts.GetEncryptionKey();
            break;

    }

    for (auto &secContext : mxSecurityContexts)
    {
        if (!secContext.is())
            continue;
        auto secEnvironment = secContext->getSecurityEnvironment();
        if (!secEnvironment.is())
            continue;

        uno::Sequence< uno::Reference< security::XCertificate > > xCerts;
        try
        {
            if ( meAction == UserAction::Sign || meAction == UserAction::SelectSign)
                xCerts = secEnvironment->getPersonalCertificates();
            else
                xCerts = secEnvironment->getAllCertificates();
        }
        catch (security::NoPasswordException&)
        {
        }

        sal_Int32 nCertificates = xCerts.getLength();
        for( sal_Int32 nCert = nCertificates; nCert; )
        {
            uno::Reference< security::XCertificate > xCert = xCerts[ --nCert ];
            // Check if we have a private key for this...
            long nCertificateCharacters = secEnvironment->getCertificateCharacters(xCert);

            if (!(nCertificateCharacters & security::CertificateCharacters::HAS_PRIVATE_KEY))
            {
                ::comphelper::removeElementAt( xCerts, nCert );
                nCertificates = xCerts.getLength();
            }
        }


        // fill list of certificates; the first entry will be selected
        for ( sal_Int32 nC = 0; nC < nCertificates; ++nC )
        {
            std::shared_ptr<UserData> userData = std::make_shared<UserData>();
            userData->xCertificate = xCerts[ nC ];
            userData->xSecurityContext = secContext;
            userData->xSecurityEnvironment = secEnvironment;
            mvUserData.push_back(userData);

            OUString sIssuer = xmlsec::GetContentPart( xCerts[ nC ]->getIssuerName() );
            SvTreeListEntry* pEntry = m_pCertLB->InsertEntry( xmlsec::GetContentPart( xCerts[ nC ]->getSubjectName() )
                + "\t" + sIssuer
                + "\t" + xmlsec::GetCertificateKind( xCerts[ nC ]->getCertificateKind() )
                + "\t" + utl::GetDateString( xCerts[ nC ]->getNotValidAfter() )
                + "\t" + UsageInClearText( xCerts[ nC ]->getCertificateUsage() ) );
            pEntry->SetUserData( userData.get() );

#if HAVE_FEATURE_GPGME
            // only GPG has preferred keys
            if ( sIssuer == msPreferredKey )
            {
                if ( meAction == UserAction::Sign || meAction == UserAction::SelectSign )
                    m_pCertLB->Select( pEntry );
                else if ( meAction == UserAction::Encrypt &&
                          aUserOpts.GetEncryptToSelf() )
                    mxEncryptToSelf = xCerts[nC];
            }
#endif
        }
    }

    // enable/disable buttons
    CertificateHighlightHdl( nullptr );
    mbInitialized = true;
}


uno::Sequence<uno::Reference< css::security::XCertificate > > CertificateChooser::GetSelectedCertificates()
{
    std::vector< uno::Reference< css::security::XCertificate > > aRet;
    SvTreeListEntry* pSel = m_pCertLB->FirstSelected();

    if (meAction == UserAction::Encrypt)
    {
        // for encryption, multiselection is enabled
        while(pSel)
        {
            UserData* userData = static_cast<UserData*>(pSel->GetUserData());
            aRet.push_back( userData->xCertificate );
            pSel = m_pCertLB->NextSelected(pSel);
        }
    }
    else
    {
        uno::Reference< css::security::XCertificate > xCert;
        if( pSel )
        {
            UserData* userData = static_cast<UserData*>(pSel->GetUserData());
            xCert = userData->xCertificate;
        }
        aRet.push_back( xCert );
    }

#if HAVE_FEATURE_GPGME
    if ( mxEncryptToSelf.is())
        aRet.push_back( mxEncryptToSelf );
#endif

    return comphelper::containerToSequence(aRet);
}

uno::Reference<xml::crypto::XXMLSecurityContext> CertificateChooser::GetSelectedSecurityContext()
{
    SvTreeListEntry* pSel = m_pCertLB->FirstSelected();
    if( !pSel )
        return uno::Reference<xml::crypto::XXMLSecurityContext>();

    UserData* userData = static_cast<UserData*>(pSel->GetUserData());
    uno::Reference<xml::crypto::XXMLSecurityContext> xCert = userData->xSecurityContext;
    return xCert;
}

OUString CertificateChooser::GetDescription()
{
    return m_pDescriptionED->GetText();
}

OUString CertificateChooser::GetUsageText()
{
    uno::Sequence< uno::Reference<css::security::XCertificate> > xCerts =
        GetSelectedCertificates();
    return (xCerts.hasElements() && xCerts[0].is()) ?
        UsageInClearText(xCerts[0]->getCertificateUsage()) : OUString();
}

IMPL_LINK_NOARG(CertificateChooser, CertificateHighlightHdl, SvTreeListBox*, void)
{
    bool bEnable = m_pCertLB->GetSelectionCount() > 0;
    m_pViewBtn->Enable( bEnable );
    m_pOKBtn->Enable( bEnable );
    m_pDescriptionED->Enable(bEnable);
}

IMPL_LINK_NOARG(CertificateChooser, CertificateSelectHdl, SvTreeListBox*, bool)
{
    EndDialog( RET_OK );
    return false;
}

IMPL_LINK_NOARG(CertificateChooser, ViewButtonHdl, Button*, void)
{
    ImplShowCertificateDetails();
}

void CertificateChooser::ImplShowCertificateDetails()
{
    SvTreeListEntry* pSel = m_pCertLB->FirstSelected();
    if( !pSel )
        return;

    UserData* userData = static_cast<UserData*>(pSel->GetUserData());

    if (!userData->xSecurityEnvironment.is() || !userData->xCertificate.is())
        return;

    ScopedVclPtrInstance< CertificateViewer > aViewer( this, userData->xSecurityEnvironment, userData->xCertificate, true );
    aViewer->Execute();
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
