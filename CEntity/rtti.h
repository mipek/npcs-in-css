/**
* =============================================================================
* CEntity Entity Handling Framework
* Copyright (C) 2011 Matt Woodrow.  All rights reserved.
* =============================================================================
*
* This program is free software; you can redistribute it and/or modify it under
* the terms of the GNU General Public License, version 3.0, as published by the
* Free Software Foundation.
* 
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
* FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
* details.
*
* You should have received a copy of the GNU General Public License along with
* this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef RTTI_H_
#define RTTI_H_

#include <typeinfo>
#include <extension.h>

class IBaseType
{
public:
	/* Offset from the original IType* */
	virtual ptrdiff_t GetOffset() =0;

	/* Gets the basic type info for this type */
	virtual const std::type_info &GetTypeInfo() =0;

	/* Returns the number of types this type inherits from */
	virtual size_t GetNumBaseClasses() =0;

	/* Gets the first base class */
	virtual IBaseType *GetBaseClass(size_t num) =0;
};

class IType
{
public:
	/* Gets the top level class this type represents */
	virtual IBaseType *GetBaseType() =0;

	/* Destroy all memory resources held by this object chain */
	virtual void Destroy() =0;
};

/* Get type information for a class pointer */
IType *GetType(const void *ptr);

/* Returns the classname for a given type - Removes platform specific formatting */
const char *GetTypeName(const std::type_info &type);

inline void DumpType(IBaseType *pType, int level)
{
	for (int i = 0; i < level; i++)
		META_CONPRINT("-");

	META_CONPRINTF("%s\n", GetTypeName(pType->GetTypeInfo()));

	for (size_t i = 0; i < pType->GetNumBaseClasses(); i++)
	{
		DumpType(pType->GetBaseClass(i), level + 1);
	}
}

inline void PrintTypeTree(void *pClass)
{
	IType *pType = GetType(pClass);
	IBaseType *pBase = pType->GetBaseType();

	DumpType(pBase, 0);

	pType->Destroy();
}

#endif // RTTI_H_
