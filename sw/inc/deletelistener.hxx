/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <svl/listener.hxx>
#include <svl/lstner.hxx>
#include "calbck.hxx"

class SwDeleteListener final : public SwClient
{
private:
    SwModify* m_pModify;

    virtual void SwClientNotify(const SwModify&, const SfxHint& rHint) override
    {
        if (auto pLegacy = dynamic_cast<const sw::LegacyModifyHint*>(&rHint))
        {
            if (pLegacy->m_pOld && pLegacy->m_pOld->Which() == RES_OBJECTDYING)
            {
                m_pModify->Remove(this);
                m_pModify = nullptr;
            }
        }
    }

public:
    SwDeleteListener(SwModify& rModify)
        : m_pModify(&rModify)
    {
        m_pModify->Add(this);
    }

    bool WasDeleted() const { return !m_pModify; }

    virtual ~SwDeleteListener() override
    {
        if (!m_pModify)
            return;
        m_pModify->Remove(this);
    }
};

class SvtDeleteListener final : public SvtListener
{
private:
    bool bObjectDeleted;

public:
    explicit SvtDeleteListener(SvtBroadcaster& rNotifier)
        : bObjectDeleted(false)
    {
        StartListening(rNotifier);
    }

    virtual void Notify(const SfxHint& rHint) override
    {
        if (rHint.GetId() == SfxHintId::Dying)
            bObjectDeleted = true;
    }

    bool WasDeleted() const { return bObjectDeleted; }
};

class SfxDeleteListener final : public SfxListener
{
private:
    bool bObjectDeleted;

public:
    explicit SfxDeleteListener(SfxBroadcaster& rNotifier)
        : bObjectDeleted(false)
    {
        StartListening(rNotifier);
    }

    virtual void Notify(SfxBroadcaster& /*rBC*/, const SfxHint& rHint) override
    {
        if (rHint.GetId() == SfxHintId::Dying)
            bObjectDeleted = true;
    }

    bool WasDeleted() const { return bObjectDeleted; }
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
