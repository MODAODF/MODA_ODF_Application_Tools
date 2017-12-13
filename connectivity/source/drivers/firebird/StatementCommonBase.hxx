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

#ifndef INCLUDED_CONNECTIVITY_SOURCE_DRIVERS_FIREBIRD_STATEMENTCOMMONBASE_HXX
#define INCLUDED_CONNECTIVITY_SOURCE_DRIVERS_FIREBIRD_STATEMENTCOMMONBASE_HXX

#include "Connection.hxx"

#include <ibase.h>

#include <connectivity/OSubComponent.hxx>
#include <cppuhelper/compbase.hxx>
#include <list>

#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/sdbc/SQLWarning.hpp>
#include <com/sun/star/sdbc/XBatchExecution.hpp>
#include <com/sun/star/sdbc/XCloseable.hpp>
#include <com/sun/star/sdbc/XMultipleResults.hpp>
#include <com/sun/star/sdbc/XStatement.hpp>
#include <com/sun/star/sdbc/XWarningsSupplier.hpp>
#include <com/sun/star/util/XCancellable.hpp>

namespace connectivity
{
    namespace firebird
    {

        typedef ::cppu::WeakComponentImplHelper<   ::com::sun::star::sdbc::XWarningsSupplier,
                                                   ::com::sun::star::util::XCancellable,
                                                   ::com::sun::star::sdbc::XCloseable,
                                                   ::com::sun::star::sdbc::XMultipleResults> OStatementCommonBase_Base;

        class OStatementCommonBase  :   public  OStatementCommonBase_Base,
                                        public  ::cppu::OPropertySetHelper,
                                        public  OPropertyArrayUsageHelper<OStatementCommonBase>

        {
        protected:
            ::osl::Mutex        m_aMutex;

            ::com::sun::star::uno::Reference< ::com::sun::star::sdbc::XResultSet>    m_xResultSet;   // The last ResultSet created
            //  for this Statement

            ::rtl::Reference<Connection>                m_pConnection;

            ISC_STATUS_ARRAY                            m_statusVector;
            isc_stmt_handle                             m_aStatementHandle;

        protected:
            virtual void disposeResultSet();
            void freeStatementHandle()
                throw (::com::sun::star::sdbc::SQLException);

            // OPropertyArrayUsageHelper
            virtual ::cppu::IPropertyArrayHelper* createArrayHelper( ) const override;
            // OPropertySetHelper
            using OPropertySetHelper::getFastPropertyValue;
            virtual ::cppu::IPropertyArrayHelper & SAL_CALL getInfoHelper() override;
            virtual sal_Bool SAL_CALL convertFastPropertyValue(
                                                                ::com::sun::star::uno::Any & rConvertedValue,
                                                                ::com::sun::star::uno::Any & rOldValue,
                                                                sal_Int32 nHandle,
                                                                const ::com::sun::star::uno::Any& rValue )
                                                            throw (::com::sun::star::lang::IllegalArgumentException) override;
            virtual void SAL_CALL setFastPropertyValue_NoBroadcast(
                                                                sal_Int32 nHandle,
                                                                const ::com::sun::star::uno::Any& rValue)   throw (::com::sun::star::uno::Exception, std::exception) override;
            virtual void SAL_CALL getFastPropertyValue(
                                                                ::com::sun::star::uno::Any& rValue,
                                                                sal_Int32 nHandle) const override;
            virtual ~OStatementCommonBase();

            void prepareAndDescribeStatement(const OUString& sqlIn,
                                             XSQLDA*& pOutSqlda,
                                             XSQLDA* pInSqlda=nullptr)
                throw (::com::sun::star::sdbc::SQLException);

            short getSqlInfoItem(char aInfoItem)
                throw (::com::sun::star::sdbc::SQLException);
            bool isDDLStatement()
                throw (::com::sun::star::sdbc::SQLException);
            sal_Int32 getStatementChangeCount()
                throw (::com::sun::star::sdbc::SQLException);

        public:

            explicit OStatementCommonBase(Connection* _pConnection);
            using OStatementCommonBase_Base::operator ::com::sun::star::uno::Reference< ::com::sun::star::uno::XInterface >;

            // OComponentHelper
            virtual void SAL_CALL disposing() override {
                disposeResultSet();
                OStatementCommonBase_Base::disposing();
            }
            // XInterface
            virtual void SAL_CALL release() throw() override;
            virtual void SAL_CALL acquire() throw() override;
            // XInterface
            virtual ::com::sun::star::uno::Any SAL_CALL queryInterface( const ::com::sun::star::uno::Type & rType ) throw(::com::sun::star::uno::RuntimeException, std::exception) override;
            //XTypeProvider
            virtual ::com::sun::star::uno::Sequence< ::com::sun::star::uno::Type > SAL_CALL getTypes(  ) throw(::com::sun::star::uno::RuntimeException, std::exception) override;

            // XPropertySet
            virtual ::com::sun::star::uno::Reference< ::com::sun::star::beans::XPropertySetInfo > SAL_CALL getPropertySetInfo(  ) throw(::com::sun::star::uno::RuntimeException, std::exception) override;

            // XWarningsSupplier - UNSUPPORTED
            virtual ::com::sun::star::uno::Any SAL_CALL getWarnings(  ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException, std::exception) override;
            virtual void SAL_CALL clearWarnings(  ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException, std::exception) override;
            // XMultipleResults - UNSUPPORTED
            virtual ::com::sun::star::uno::Reference< ::com::sun::star::sdbc::XResultSet > SAL_CALL getResultSet(  ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException, std::exception) override;
            virtual sal_Int32 SAL_CALL getUpdateCount(  ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException, std::exception) override;
            virtual sal_Bool SAL_CALL getMoreResults(  ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException, std::exception) override;

            // XCancellable
            virtual void SAL_CALL cancel(  ) throw(::com::sun::star::uno::RuntimeException, std::exception) override;
            // XCloseable
            virtual void SAL_CALL close(  ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException, std::exception) override;

        };
    }
}

#endif // INCLUDED_CONNECTIVITY_SOURCE_DRIVERS_FIREBIRD_STATEMENTCOMMONBASE_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
