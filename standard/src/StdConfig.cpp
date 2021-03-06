/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Auto-registering data structures */

#include <Standard.h>
#include <StdRegistry.h>
#include <StdConfig.h>

#include <stdio.h>

CStdConfig::CStdConfig()
	{

	}

CStdConfig::~CStdConfig()
	{

	}

BOOL CStdConfig::Load(CStdConfigValue *pCfgMap, void *vpData)
	{
#ifdef _WIN32
	if (!pCfgMap || !vpData) return FALSE;

	char szCompany[100+1]="Company";
	char szProduct[100+1]="Product";
	char szSection[100+1]="Section";

	char szSubkey[1024+1];
	DWORD dwValue;
	char szValue[CFG_MaxString+1];

	for (; pCfgMap && (pCfgMap->Type!=CFG_End); pCfgMap++)
		switch (pCfgMap->Type)
			{
			case CFG_Company: SCopy(pCfgMap->Name,szCompany,100); break;
			case CFG_Section: SCopy(pCfgMap->Name,szSection,100); break;
			case CFG_Product: SCopy(pCfgMap->Name,szProduct,100); break;
			case CFG_Integer:
				sprintf(szSubkey,"Software\\%s\\%s\\%s",szCompany,szProduct,szSection);
				if (!pCfgMap->Name || !GetRegistryDWord(szSubkey,pCfgMap->Name,&dwValue))
					dwValue=pCfgMap->Default;
				*((int*)(((BYTE*)vpData)+pCfgMap->Offset)) = dwValue;
				break;
			case CFG_String:
				sprintf(szSubkey,"Software\\%s\\%s\\%s",szCompany,szProduct,szSection);
				if (!pCfgMap->Name || !GetRegistryString(szSubkey,pCfgMap->Name,szValue,CFG_MaxString))
					SCopy((char*)pCfgMap->Default,szValue,CFG_MaxString);
				SCopy(szValue,((char*)vpData)+pCfgMap->Offset);
				break;
			}
#endif
	return TRUE;
	}

BOOL CStdConfig::Save(CStdConfigValue *pCfgMap, void *vpData)
	{
#ifdef _WIN32
	if (!pCfgMap || !vpData) return FALSE;

	char szCompany[100+1]="Company";
	char szProduct[100+1]="Product";
	char szSection[100+1]="Section";

	char szSubkey[1024+1];

	for (; pCfgMap && (pCfgMap->Type!=CFG_End); pCfgMap++)
		switch (pCfgMap->Type)
			{
			case CFG_Company: SCopy(pCfgMap->Name,szCompany,100); break;
			case CFG_Section: SCopy(pCfgMap->Name,szSection,100); break;
			case CFG_Product: SCopy(pCfgMap->Name,szProduct,100); break;
			case CFG_Integer:
				sprintf(szSubkey,"Software\\%s\\%s\\%s",szCompany,szProduct,szSection);
				if (pCfgMap->Name)
					SetRegistryDWord(szSubkey,pCfgMap->Name, *((int*)(((BYTE*)vpData)+pCfgMap->Offset)) );
				break;
			case CFG_String:
				// Compose value path
				sprintf(szSubkey,"%s",szSection);
				// Write if value
				if (*(((char*)vpData)+pCfgMap->Offset))
					{
					if (pCfgMap->Name) 
						SetRegistryString(szSubkey,pCfgMap->Name, ((char*)vpData)+pCfgMap->Offset );
					}
				// Else delete value
				else
					{
					DeleteRegistryValue(szSubkey,pCfgMap->Name);
					}
				break;
			}
#endif
	return TRUE;
	}

void CStdConfig::LoadDefault(CStdConfigValue *pCfgMap, void *vpData, const char *szOnlySection)
	{
	if (!pCfgMap || !vpData) return;

	char szCompany[100+1]="Company";
	char szProduct[100+1]="Product";
	char szSection[100+1]="Section";

	for (; pCfgMap && (pCfgMap->Type!=CFG_End); pCfgMap++)
		switch (pCfgMap->Type)
			{
			case CFG_Company: SCopy(pCfgMap->Name,szCompany,100); break;
			case CFG_Section: SCopy(pCfgMap->Name,szSection,100); break;
			case CFG_Product: SCopy(pCfgMap->Name,szProduct,100); break;
			case CFG_Integer:
				if (!szOnlySection || SEqual(szSection,szOnlySection))
					*((int*)(((BYTE*)vpData)+pCfgMap->Offset)) = pCfgMap->Default;
				break;
			case CFG_String:
				if (!szOnlySection || SEqual(szSection,szOnlySection))
					SCopy((char*)pCfgMap->Default,((char*)vpData)+pCfgMap->Offset,CFG_MaxString);
				break;
			}
	}
