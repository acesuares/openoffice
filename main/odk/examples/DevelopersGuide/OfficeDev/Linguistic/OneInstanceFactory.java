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



import com.sun.star.lang.XSingleServiceFactory;
import com.sun.star.lang.XServiceInfo;
import com.sun.star.lang.XInitialization;
import com.sun.star.lang.XMultiServiceFactory;
import com.sun.star.beans.XPropertySet;
import com.sun.star.uno.XInterface;
import com.sun.star.uno.Any;
import com.sun.star.uno.UnoRuntime;

import java.lang.reflect.Constructor;

//
// purpose of this class is to provide a service factory that instantiates
// the services only once (as long as this factory itself exists) 
// and returns only reference to that instance.
// 

public class OneInstanceFactory implements
        XSingleServiceFactory,
        XServiceInfo
{
    Class       aMyClass;
    String      aSvcImplName;
    String[]    aSupportedSvcNames;
    XInterface  xInstantiatedService;
    XMultiServiceFactory    xMultiFactory;

    public OneInstanceFactory(
            Class       aMyClass,
            String      aSvcImplName,
            String[]    aSupportedSvcNames,
            XMultiServiceFactory    xMultiFactory )
    {
        this.aMyClass           = aMyClass;
        this.aSvcImplName       = aSvcImplName;
        this.aSupportedSvcNames = aSupportedSvcNames;
        this.xMultiFactory      = xMultiFactory;
        xInstantiatedService = null;
    }

    //**********************
    // XSingleServiceFactory
    //**********************
    public Object createInstance() 
        throws com.sun.star.uno.Exception, 
               com.sun.star.uno.RuntimeException
    {
        if (xInstantiatedService == null)
        {
            //!! the here used services all have exact one constructor!!
            Constructor [] aCtor = aMyClass.getConstructors();
            try {
                xInstantiatedService = (XInterface) aCtor[0].newInstance( (Object[])null );
            }
            catch( Exception e ) {
            }
        
            //!! workaround for services not always being created
            //!! via 'createInstanceWithArguments'
            XInitialization xIni = (XInitialization) UnoRuntime.queryInterface(
                XInitialization.class, createInstance());
            if (xIni != null)
            {
                Object[] aArguments = new Object[]{ null, null };
                if (xMultiFactory != null)
                {
                    XPropertySet xPropSet = (XPropertySet) UnoRuntime.queryInterface(
                        XPropertySet.class ,  xMultiFactory.createInstance(
                            "com.sun.star.linguistic2.LinguProperties" ) );
                    aArguments[0] = xPropSet;
                }                            
                xIni.initialize( aArguments );
            }
        }
        return xInstantiatedService;
    }

    public Object createInstanceWithArguments( Object[] aArguments ) 
        throws com.sun.star.uno.Exception, 
               com.sun.star.uno.RuntimeException
    {
        if (xInstantiatedService == null)
        {
            XInitialization xIni = (XInitialization) UnoRuntime.queryInterface(
                XInitialization.class, createInstance());
            if (xIni != null)
                xIni.initialize( aArguments );
        }
        return xInstantiatedService;
    }


    //*************
    // XServiceInfo
    //*************
    public boolean supportsService( String aServiceName )
        throws com.sun.star.uno.RuntimeException
    {
        boolean bFound = false;
        int nCnt = aSupportedSvcNames.length;
        for (int i = 0;  i < nCnt && !bFound;  ++i)
        {
            if (aServiceName.equals( aSupportedSvcNames[i] ))
                bFound = true;
        }
        return bFound;
    }

    public String getImplementationName()
        throws com.sun.star.uno.RuntimeException
    {
        return aSvcImplName;
    }
        
    public String[] getSupportedServiceNames()
        throws com.sun.star.uno.RuntimeException
    {
        return aSupportedSvcNames;
    }
};

