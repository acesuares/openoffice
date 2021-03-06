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



#ifndef ARY_IDL_INTERNALGATE_HXX
#define ARY_IDL_INTERNALGATE_HXX

// BASE CLASSES
#include <ary/idl/i_gate.hxx>

namespace ary
{
    class RepositoryCenter;
}




namespace ary
{
namespace idl
{


/** Provides access to the ->idl::RepositoryPartition as far as is needed
    by the ->RepositoryCenter.
*/
class InternalGate : public ::ary::idl::Gate
{
  public:
    virtual             ~InternalGate() {}

    static DYN InternalGate &
                        Create_Partition_(
                            RepositoryCenter &  i_center );
};




}   //  namespace idl
}   //  namespace ary
#endif
