/****************************************************************************
*                                                                           *
*  OpenNI 1.x Alpha                                                         *
*  Copyright (C) 2011 PrimeSense Ltd.                                       *
*                                                                           *
*  This file is part of OpenNI.                                             *
*                                                                           *
*  OpenNI is free software: you can redistribute it and/or modify           *
*  it under the terms of the GNU Lesser General Public License as published *
*  by the Free Software Foundation, either version 3 of the License, or     *
*  (at your option) any later version.                                      *
*                                                                           *
*  OpenNI is distributed in the hope that it will be useful,                *
*  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the             *
*  GNU Lesser General Public License for more details.                      *
*                                                                           *
*  You should have received a copy of the GNU Lesser General Public License *
*  along with OpenNI. If not, see <http://www.gnu.org/licenses/>.           *
*                                                                           *
****************************************************************************/
#ifndef __EXPORTED_RECORDER_H__
#define __EXPORTED_RECORDER_H__

//---------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------
#include <XnModuleCppInterface.h>
#include <XnTypes.h>

//---------------------------------------------------------------------------
// Types
//---------------------------------------------------------------------------
class ExportedRecorder : public xn::ModuleExportedProductionNode
{
public:
	ExportedRecorder();
	virtual ~ExportedRecorder();

	virtual void GetDescription(XnProductionNodeDescription* pDescription);
	virtual XnStatus EnumerateProductionTrees(xn::Context& context, xn::NodeInfoList& TreesList, xn::EnumerationErrors* pErrors);
	virtual XnStatus Create(xn::Context& context, const XnChar* strInstanceName, const XnChar* strCreationInfo, xn::NodeInfoList* pNeededTrees, const XnChar* strConfigurationDir, xn::ModuleProductionNode** ppInstance);
	virtual void Destroy(xn::ModuleProductionNode* pInstance);

private:
	//---------------------------------------------------------------------------
	// Constants
	//---------------------------------------------------------------------------
	static const char NAME[];
	static const char CREATION_INFO[];
	
	//---------------------------------------------------------------------------
	// Member Variables
	//---------------------------------------------------------------------------
	char m_strInstanceName[XN_MAX_NAME_LENGTH];
};

#endif // __EXPORTED_RECORDER_H__