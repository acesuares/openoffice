/**************************************************************
 * 
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 * 
 *************************************************************/

#include "macavf_manager.hxx"
#include "macavf_player.hxx"

#include <tools/urlobj.hxx>

using namespace ::com::sun::star;

namespace avmedia { namespace macavf {

// ----------------
// - Manager -
// ----------------

Manager::Manager( const uno::Reference< lang::XMultiServiceFactory >& rxMgr ) :
    mxMgr( rxMgr )
{
    OSL_TRACE( "Constructing avmedia::macavf::Manager" );
}

// ------------------------------------------------------------------------------

Manager::~Manager()
{}

// ------------------------------------------------------------------------------

uno::Reference< media::XPlayer > SAL_CALL Manager::createPlayer( const ::rtl::OUString& rURL )
    throw (uno::RuntimeException)
{
    Player*                             pPlayer( new Player( mxMgr ) );
    uno::Reference< media::XPlayer >    xRet( pPlayer );
    INetURLObject                       aURL( rURL );

    OSL_TRACE( "avmediamacavf: Manager::createPlayer" );

    if( !pPlayer->create( aURL.GetMainURL( INetURLObject::DECODE_UNAMBIGUOUS ) )  )
        xRet = uno::Reference< media::XPlayer >();

    return xRet;
}

// ------------------------------------------------------------------------------

::rtl::OUString SAL_CALL Manager::getImplementationName(  )
    throw (uno::RuntimeException)
{
    return ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( AVMEDIA_MACAVF_MANAGER_IMPLEMENTATIONNAME ) );
}

// ------------------------------------------------------------------------------

sal_Bool SAL_CALL Manager::supportsService( const ::rtl::OUString& ServiceName )
    throw (uno::RuntimeException)
{
    return ServiceName.equalsAsciiL( RTL_CONSTASCII_STRINGPARAM ( AVMEDIA_MACAVF_MANAGER_SERVICENAME ) );
}

// ------------------------------------------------------------------------------

uno::Sequence< ::rtl::OUString > SAL_CALL Manager::getSupportedServiceNames(  )
    throw (uno::RuntimeException)
{
    uno::Sequence< ::rtl::OUString > aRet(1);
    aRet[0] = ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM ( AVMEDIA_MACAVF_MANAGER_SERVICENAME ) );

    return aRet;
}

} // namespace macavf
} // namespace avmedia
