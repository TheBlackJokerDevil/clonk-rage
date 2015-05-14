/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Core component of a scenario file */

#include "C4Include.h"
#include "landscape/C4Scenario.h"
#include "C4InputValidation.h"

#ifndef BIG_C4INCLUDE
#include "C4Random.h"
#include "c4group/C4Group.h"
#include "C4Components.h"
#include "game/C4Game.h"
#ifdef C4ENGINE
#include "C4Wrappers.h"
#endif
#endif

#if defined(C4FRONTEND) || defined (C4GROUP)
#include "C4CompilerWrapper.h"
#endif

//==================================== C4SVal ==============================================

C4SVal::C4SVal(int32_t std, int32_t rnd, int32_t min, int32_t max)
  : Std(std), Rnd(rnd), Min(min), Max(max)
  {
  }

void C4SVal::Set(int32_t std, int32_t rnd, int32_t min, int32_t max)
  {
  Std=std; Rnd=rnd; Min=min; Max=max;
  }

int32_t C4SVal::Evaluate()
  {
  return BoundBy(Std+Random(2*Rnd+1)-Rnd,Min,Max);
  }

void C4SVal::Default()
	{
	Set();
	}

void C4SVal::CompileFunc(StdCompiler *pComp)
  {
  pComp->Value(mkDefaultAdapt(Std, 0));
  if (!pComp->Seperator()) return;
  pComp->Value(mkDefaultAdapt(Rnd, 0));
  if (!pComp->Seperator()) return;
  pComp->Value(mkDefaultAdapt(Min, 0));
  if (!pComp->Seperator()) return;
  pComp->Value(mkDefaultAdapt(Max, 100));
  }

//================================ C4Scenario ==========================================

C4Scenario::C4Scenario()
  {
  Default();
  }

void C4Scenario::Default()
  {
  int32_t cnt;
  Head.Default();
	Definitions.Default();
  Game.Default();
  for (cnt=0; cnt<C4S_MaxPlayer; cnt++) PlrStart[cnt].Default();
  Landscape.Default();
  Animals.Default();
  Weather.Default();
  Disasters.Default();
  Game.Realism.Default();
	Environment.Default();
  }

BOOL C4Scenario::Load(C4Group &hGroup, bool fLoadSection)
  {
	char *pSource;
	// Load
	if (!hGroup.LoadEntry(C4CFN_ScenarioCore,&pSource,NULL,1)) return FALSE;
	// Compile
	if (!Compile(pSource, fLoadSection))	{ delete [] pSource; return FALSE; }
	delete [] pSource;
	// Convert
	Game.ConvertGoals(Game.Realism);
	// Success
	return TRUE;
  }

BOOL C4Scenario::Save(C4Group &hGroup, bool fSaveSection)
	{
	char *Buffer; int32_t BufferSize;
	if (!Decompile(&Buffer,&BufferSize, fSaveSection)) 
		return FALSE;
	if (!hGroup.Add(C4CFN_ScenarioCore,Buffer,BufferSize,FALSE,TRUE))
		{ StdBuf Buf; Buf.Take(Buffer, BufferSize); return FALSE; }
	return TRUE;
	}

void C4Scenario::CompileFunc(StdCompiler *pComp, bool fSection)
  {
  pComp->Value(mkNamingAdapt(mkParAdapt(Head, fSection), "Head"));
  if (!fSection) pComp->Value(mkNamingAdapt(Definitions, "Definitions"));
	pComp->Value(mkNamingAdapt(mkParAdapt(Game, fSection), "Game"));
  for(int32_t i = 0; i < C4S_MaxPlayer; i++)
    pComp->Value(mkNamingAdapt(PlrStart[i], FormatString("Player%d", i+1).getData()));
  pComp->Value(mkNamingAdapt(Landscape, "Landscape"));
  pComp->Value(mkNamingAdapt(Animals, "Animals"));
  pComp->Value(mkNamingAdapt(Weather, "Weather"));
  pComp->Value(mkNamingAdapt(Disasters, "Disasters"));
  pComp->Value(mkNamingAdapt(Environment, "Environment"));  
  }

int32_t C4Scenario::GetMinPlayer()
	{
	// MinPlayer is specified.
	if (Head.MinPlayer != 0) 
		return Head.MinPlayer;
	// Melee? Need at least two.
	if (Game.IsMelee())
		return 2;
	// Otherwise/unknown: need at least one.
	return 1;
	}

void C4SDefinitions::Default()	
  {
	LocalOnly=AllowUserChange=FALSE;
	ZeroMem(Definition,sizeof (Definition));
	SkipDefs.Default();
	}

const int32_t C4S_MaxPlayerDefault = 12;

void C4SHead::Default()
  {
	Origin.Clear();
	ZeroMem(this,sizeof (C4SHead));
  Icon=18;
	MaxPlayer=MaxPlayerLeague=C4S_MaxPlayerDefault;
	MinPlayer=0; // auto-determine by mode
	RandomSeed = 0;
	ForcedAutoContextMenu = -1;
	ForcedControlStyle = -1;
  SCopy("Default Title",Title,C4MaxTitle);
  }

void C4SHead::CompileFunc(StdCompiler *pComp, bool fSection)
  {
	if (!fSection)
		{
		pComp->Value(mkNamingAdapt(Icon,                      "Icon",                 18));
		pComp->Value(mkNamingAdapt(mkStringAdaptMA(Title),    "Title",                "Default Title"));
		pComp->Value(mkNamingAdapt(mkStringAdaptMA(Loader),   "Loader",               ""));
		pComp->Value(mkNamingAdapt(mkStringAdaptMA(Font),     "Font",                 ""));
		pComp->Value(mkNamingAdapt(mkArrayAdaptDM(C4XVer,0),  "Version"               ));
		pComp->Value(mkNamingAdapt(Difficulty,								"Difficulty",						0));
		pComp->Value(mkNamingAdapt(EnableUnregisteredAccess,  "Access",               FALSE));
		pComp->Value(mkNamingAdapt(MaxPlayer,                 "MaxPlayer",            C4S_MaxPlayerDefault));
		pComp->Value(mkNamingAdapt(MaxPlayerLeague,           "MaxPlayerLeague",      MaxPlayer));
		pComp->Value(mkNamingAdapt(MinPlayer,                 "MinPlayer",            0));
		pComp->Value(mkNamingAdapt(SaveGame,                  "SaveGame",             FALSE));
		pComp->Value(mkNamingAdapt(Replay,                    "Replay",               FALSE));
		pComp->Value(mkNamingAdapt(Film,                      "Film",                 FALSE));
		pComp->Value(mkNamingAdapt(DisableMouse,              "DisableMouse",         FALSE));
		pComp->Value(mkNamingAdapt(IgnoreSyncChecks,          "IgnoreSyncChecks",     FALSE));
		}
  pComp->Value(mkNamingAdapt(NoInitialize,              "NoInitialize",         FALSE));
	pComp->Value(mkNamingAdapt(RandomSeed,                "RandomSeed",         0));
	if (!fSection)
		{
	  pComp->Value(mkNamingAdapt(mkStringAdaptMA(Engine),   "Engine",               ""));
	  pComp->Value(mkNamingAdapt(mkStringAdaptMA(MissionAccess), "MissionAccess", ""));
	  pComp->Value(mkNamingAdapt(NetworkGame,               "NetworkGame",          false));
		pComp->Value(mkNamingAdapt(NetworkRuntimeJoin,        "NetworkRuntimeJoin",   false));
	  pComp->Value(mkNamingAdapt(ForcedGfxMode,							"ForcedGfxMode",         0));
	  pComp->Value(mkNamingAdapt(ForcedFairCrew,            "ForcedNoCrew",          0));
	  pComp->Value(mkNamingAdapt(FairCrewStrength,          "DefCrewStrength",       0));
	  pComp->Value(mkNamingAdapt(ForcedAutoContextMenu,     "ForcedAutoContextMenu",         -1));
	  pComp->Value(mkNamingAdapt(ForcedControlStyle,        "ForcedAutoStopControl",          -1));
		pComp->Value(mkNamingAdapt(mkStrValAdapt(mkParAdapt(Origin, StdCompiler::RCT_All), C4InVal::VAL_SubPathFilename),	 "Origin",  StdCopyStrBuf()));
		// windows needs backslashes in Origin; other systems use forward slashes
		if (pComp->isCompiler()) Origin.ReplaceChar(AltDirectorySeparator, DirectorySeparator);
		}
  }

void C4SGame::Default()
  {
  Elimination=C4S_EliminateCrew;    
  ValueGain=0;
	CreateObjects.Clear();
	ClearObjects.Clear();
	ClearMaterial.Clear();
  Mode=C4S_Cooperative;
	CooperativeGoal=C4S_NoGoal;
  EnableRemoveFlag=0;
	Goals.Clear();
	Rules.Clear();
	FoWColor=0;
  }

void C4SGame::CompileFunc(StdCompiler *pComp, bool fSection)
  {
	pComp->Value(mkNamingAdapt(Mode,                     "Mode",                C4S_Cooperative));
	pComp->Value(mkNamingAdapt(Elimination,              "Elimination",         C4S_EliminateCrew));
	pComp->Value(mkNamingAdapt(CooperativeGoal,          "CooperativeGoal",     C4S_NoGoal));
	pComp->Value(mkNamingAdapt(CreateObjects,            "CreateObjects",       C4IDList()));
	pComp->Value(mkNamingAdapt(ClearObjects,             "ClearObjects",        C4IDList()));
	pComp->Value(mkNamingAdapt(ClearMaterial,            "ClearMaterials",      C4NameList()));
	pComp->Value(mkNamingAdapt(ValueGain,                "ValueGain",           0));
	pComp->Value(mkNamingAdapt(EnableRemoveFlag,         "EnableRemoveFlag",    FALSE));
	pComp->Value(mkNamingAdapt(Realism.ConstructionNeedsMaterial, "StructNeedMaterial",  FALSE));
	pComp->Value(mkNamingAdapt(Realism.StructuresNeedEnergy,      "StructNeedEnergy",    TRUE));
	if (!fSection)
		{
		pComp->Value(mkNamingAdapt(Realism.ValueOverloads,            "ValueOverloads",      C4IDList()));
		}
	pComp->Value(mkNamingAdapt(mkRuntimeValueAdapt(Realism.LandscapePushPull),         "LandscapePushPull",   FALSE));
	pComp->Value(mkNamingAdapt(mkRuntimeValueAdapt(Realism.LandscapeInsertThrust),     "LandscapeInsertThrust",TRUE));

	const StdBitfieldEntry<int32_t> BaseFunctionalities[] = {
		{ "BASEFUNC_AutoSellContents",								BASEFUNC_AutoSellContents		},
		{ "BASEFUNC_RegenerateEnergy",								BASEFUNC_RegenerateEnergy		},
		{ "BASEFUNC_Buy",															BASEFUNC_Buy								},
		{ "BASEFUNC_Sell",														BASEFUNC_Sell								},
		{ "BASEFUNC_RejectEntrance",									BASEFUNC_RejectEntrance			},
		{ "BASEFUNC_Extinguish",											BASEFUNC_Extinguish					},
		{ "BASEFUNC_Default",													BASEFUNC_Default						},
		{ NULL, 0 } };

	pComp->Value(mkNamingAdapt(mkRuntimeValueAdapt(mkBitfieldAdapt<int32_t>(Realism.BaseFunctionality, BaseFunctionalities)), "BaseFunctionality",   BASEFUNC_Default));
	pComp->Value(mkNamingAdapt(mkRuntimeValueAdapt(Realism.BaseRegenerateEnergyPrice), "BaseRegenerateEnergyPrice",BASE_RegenerateEnergyPrice));
  pComp->Value(mkNamingAdapt(Goals,                    "Goals",               C4IDList()));
  pComp->Value(mkNamingAdapt(Rules,                    "Rules",               C4IDList()));
  pComp->Value(mkNamingAdapt(FoWColor,                 "FoWColor",            0u));
  }

void C4SGame::ClearCooperativeGoals()
	{
	ValueGain=0;
	CreateObjects.Clear();
	ClearObjects.Clear();
	ClearMaterial.Clear();
	}

void C4SPlrStart::Default()
  {
	NativeCrew=C4ID_None;
  Crew.Set(1,0,1,10);
  Wealth.Set(0,0,0,250);
  Position[0]=Position[1]=-1;
	EnforcePosition=0;
	ReadyCrew.Default();
  ReadyBase.Default();
  ReadyVehic.Default();
  ReadyMaterial.Default();
  BuildKnowledge.Default();
  HomeBaseMaterial.Default();
  HomeBaseProduction.Default();
	Magic.Default();
  }

bool C4SPlrStart::EquipmentEqual(C4SPlrStart &rhs)
	{
	return *this == rhs;
	}

bool C4SPlrStart::operator==(const C4SPlrStart& rhs)
	{
	return (NativeCrew==rhs.NativeCrew)
		&& (Crew == rhs.Crew)
		&& (Wealth == rhs.Wealth)
		&& (ReadyCrew == rhs.ReadyCrew)
		&& (ReadyBase == rhs.ReadyBase)
		&& (ReadyVehic == rhs.ReadyVehic)
		&& (ReadyMaterial == rhs.ReadyMaterial)
		&& (BuildKnowledge == rhs.BuildKnowledge)
		&& (HomeBaseMaterial == rhs.HomeBaseMaterial)
		&& (HomeBaseProduction == rhs.HomeBaseProduction)
		&& (Magic == rhs.Magic);
	}

void C4SPlrStart::CompileFunc(StdCompiler *pComp)
  {
  pComp->Value(mkNamingAdapt(mkC4IDAdapt(NativeCrew), "StandardCrew",          C4ID_None));
  pComp->Value(mkNamingAdapt(Crew,                    "Clonks",                C4SVal(1, 0, 1, 10), true));
  pComp->Value(mkNamingAdapt(Wealth,                  "Wealth",                C4SVal(0, 0, 0,250), true));
  pComp->Value(mkNamingAdapt(mkArrayAdaptDM(Position,-1), "Position"           ));
  pComp->Value(mkNamingAdapt(EnforcePosition,         "EnforcePosition",       0));
  pComp->Value(mkNamingAdapt(ReadyCrew,               "Crew",                  C4IDList()));
  pComp->Value(mkNamingAdapt(ReadyBase,               "Buildings",             C4IDList()));
  pComp->Value(mkNamingAdapt(ReadyVehic,              "Vehicles",              C4IDList()));
  pComp->Value(mkNamingAdapt(ReadyMaterial,           "Material",              C4IDList()));
  pComp->Value(mkNamingAdapt(BuildKnowledge,          "Knowledge",             C4IDList()));
  pComp->Value(mkNamingAdapt(HomeBaseMaterial,        "HomeBaseMaterial",      C4IDList()));
  pComp->Value(mkNamingAdapt(HomeBaseProduction,      "HomeBaseProduction",    C4IDList()));
  pComp->Value(mkNamingAdapt(Magic,                   "Magic",                 C4IDList()));
  }

void C4SLandscape::Default()
  {
  BottomOpen=0; TopOpen=1;
  LeftOpen=0; RightOpen=0;
  AutoScanSideOpen=1;
  SkyDef[0]=0;
	NoSky=0;
  for (int32_t cnt=0; cnt<6; cnt++) SkyDefFade[cnt]=0;
  VegLevel.Set(50,30,0,100);
  Vegetation.Default();
  InEarthLevel.Set(50,0,0,100);
  InEarth.Default();
  MapWdt.Set(100,0,64,250);
  MapHgt.Set(50,0,40,250);
  MapZoom.Set(10,0,5,15);
  Amplitude.Set(0,0);
  Phase.Set(50);
  Period.Set(15);
  Random.Set(0);
  LiquidLevel.Default();
  MapPlayerExtend=0;
  Layers.Clear();
	SCopy("Earth",Material,C4M_MaxName);
	SCopy("Water",Liquid,C4M_MaxName);
	ExactLandscape=0;
	Gravity.Set(100,0,10,200);
	NoScan=0;
	KeepMapCreator=0;
	SkyScrollMode=0;
	NewStyleLandscape=0;
	FoWRes=CClrModAddMap::iDefResolutionX;
	}

void C4SLandscape::GetMapSize(int32_t &rWdt, int32_t &rHgt, int32_t iPlayerNum)
	{
	rWdt = MapWdt.Evaluate();
	rHgt = MapHgt.Evaluate();
	iPlayerNum = Max<int32_t>( iPlayerNum, 1 );
	if (MapPlayerExtend) 
		rWdt = Min(rWdt * Min(iPlayerNum, C4S_MaxMapPlayerExtend), MapWdt.Max);
	}

void C4SLandscape::CompileFunc(StdCompiler *pComp)
  {
  pComp->Value(mkNamingAdapt(ExactLandscape,          "ExactLandscape",        FALSE));
  pComp->Value(mkNamingAdapt(Vegetation,              "Vegetation",            C4IDList()));
  pComp->Value(mkNamingAdapt(VegLevel,                "VegetationLevel",       C4SVal(50,30,0,100), true));
  pComp->Value(mkNamingAdapt(InEarth,                 "InEarth",               C4IDList()));
  pComp->Value(mkNamingAdapt(InEarthLevel,            "InEarthLevel",          C4SVal(50,0,0,100), true));
  pComp->Value(mkNamingAdapt(mkStringAdaptMA(SkyDef), "Sky",                   ""));
  pComp->Value(mkNamingAdapt(mkArrayAdaptDM(SkyDefFade,0),"SkyFade"            ));
  pComp->Value(mkNamingAdapt(NoSky,                   "NoSky",                 FALSE));
  pComp->Value(mkNamingAdapt(BottomOpen,              "BottomOpen",            FALSE));
  pComp->Value(mkNamingAdapt(TopOpen,                 "TopOpen",               TRUE));
  pComp->Value(mkNamingAdapt(LeftOpen,                "LeftOpen",              0));
  pComp->Value(mkNamingAdapt(RightOpen,               "RightOpen",             0));
  pComp->Value(mkNamingAdapt(AutoScanSideOpen,        "AutoScanSideOpen",      TRUE));
  pComp->Value(mkNamingAdapt(MapWdt,                  "MapWidth",              C4SVal(100,0,64,250), true));
  pComp->Value(mkNamingAdapt(MapHgt,                  "MapHeight",             C4SVal(50,0,40,250), true));
	pComp->Value(mkNamingAdapt(MapZoom,                 "MapZoom",               C4SVal(10,0,5,15), true));
  pComp->Value(mkNamingAdapt(Amplitude,               "Amplitude",             C4SVal(0)));
  pComp->Value(mkNamingAdapt(Phase,                   "Phase",                 C4SVal(50)));
  pComp->Value(mkNamingAdapt(Period,                  "Period",                C4SVal(15)));
  pComp->Value(mkNamingAdapt(Random,                  "Random",                C4SVal(0)));
  pComp->Value(mkNamingAdapt(mkStringAdaptMA(Material),"Material",             "Earth"));
  pComp->Value(mkNamingAdapt(mkStringAdaptMA(Liquid), "Liquid",                "Water"));
  pComp->Value(mkNamingAdapt(LiquidLevel,             "LiquidLevel",           C4SVal()));
  pComp->Value(mkNamingAdapt(MapPlayerExtend,         "MapPlayerExtend",       0));
  pComp->Value(mkNamingAdapt(Layers,                  "Layers",                C4NameList()));
  pComp->Value(mkNamingAdapt(Gravity,                 "Gravity",               C4SVal(100,0,10,200), true));
  pComp->Value(mkNamingAdapt(NoScan,                  "NoScan",                FALSE));
  pComp->Value(mkNamingAdapt(KeepMapCreator,          "KeepMapCreator",        FALSE));
  pComp->Value(mkNamingAdapt(SkyScrollMode,           "SkyScrollMode",         0));
  pComp->Value(mkNamingAdapt(NewStyleLandscape,       "NewStyleLandscape",     FALSE));
	pComp->Value(mkNamingAdapt(FoWRes,                  "FoWRes",                static_cast<int32_t>(CClrModAddMap::iDefResolutionX)));
  }

void C4SWeather::Default()
  {
  Climate.Set(50,10);
  StartSeason.Set(50,50);
  YearSpeed.Set(50);
  Rain.Default(); Lightning.Default(); Wind.Set(0,70,-100,+100);
	SCopy("Water",Precipitation,C4M_MaxName);
	NoGamma=1;
  }

void C4SWeather::CompileFunc(StdCompiler *pComp)
  {
  pComp->Value(mkNamingAdapt(Climate,                 "Climate",               C4SVal(50,10), true));
  pComp->Value(mkNamingAdapt(StartSeason,             "StartSeason",           C4SVal(50,50), true));
  pComp->Value(mkNamingAdapt(YearSpeed,               "YearSpeed",               C4SVal(50)));
  pComp->Value(mkNamingAdapt(Rain,                    "Rain",                  C4SVal()));
  pComp->Value(mkNamingAdapt(Wind,                    "Wind",                  C4SVal(0,70,-100,+100), true));
  pComp->Value(mkNamingAdapt(Lightning,               "Lightning",             C4SVal()));
  pComp->Value(mkNamingAdapt(mkStringAdaptMA(Precipitation),"Precipitation",   "Water"));
  pComp->Value(mkNamingAdapt(NoGamma,                 "NoGamma",               TRUE));
  }

void C4SAnimals::Default()
  {
  FreeLife.Clear();
  EarthNest.Clear();
  }

void C4SAnimals::CompileFunc(StdCompiler *pComp)
  {
  pComp->Value(mkNamingAdapt(FreeLife,                "Animal",               C4IDList()));
  pComp->Value(mkNamingAdapt(EarthNest,               "Nest",                  C4IDList()));
  }

void C4SEnvironment::Default()
  {
  Objects.Clear();
  }

void C4SEnvironment::CompileFunc(StdCompiler *pComp)
  {
  pComp->Value(mkNamingAdapt(Objects,                 "Objects",               C4IDList()));
  }

void C4SRealism::Default()
  {
  ConstructionNeedsMaterial=0;
  StructuresNeedEnergy=1;
	LandscapePushPull=0;
	LandscapeInsertThrust=0;
	ValueOverloads.Default();
	BaseFunctionality = BASEFUNC_Default;
	BaseRegenerateEnergyPrice = BASE_RegenerateEnergyPrice;
  }

void C4SDisasters::Default()
  {
  Volcano.Default();
  Earthquake.Default();
  Meteorite.Default();
  }

void C4SDisasters::CompileFunc(StdCompiler *pComp)
  {
  pComp->Value(mkNamingAdapt(Meteorite,               "Meteorite",             C4SVal()));
  pComp->Value(mkNamingAdapt(Volcano,                 "Volcano",               C4SVal()));
  pComp->Value(mkNamingAdapt(Earthquake,              "Earthquake",            C4SVal()));  
  }

BOOL C4Scenario::Compile(const char *szSource, bool fLoadSection)
	{
	if (!fLoadSection) Default();
  return CompileFromBuf_LogWarn<StdCompilerINIRead>(mkParAdapt(*this, fLoadSection), StdStrBuf(szSource), C4CFN_ScenarioCore);
	}

BOOL C4Scenario::Decompile(char **ppOutput, int32_t *ipSize, bool fSaveSection)
	{
  try
    {
    // Decompile
    StdStrBuf Buf = DecompileToBuf<StdCompilerINIWrite>(mkParAdapt(*this, fSaveSection));
    // Return
    *ppOutput = Buf.GrabPointer();
    *ipSize = Buf.getSize();
    }
  catch(StdCompiler::Exception *)
    { return FALSE; }
  return TRUE;
	}

void C4Scenario::Clear()
	{

	}

void C4Scenario::SetExactLandscape()
	{
	if (Landscape.ExactLandscape) return;
	//int32_t iMapZoom = Landscape.MapZoom.Std;
	// Set landscape
	Landscape.ExactLandscape = 1;
	/*FIXME: warum ist das auskommentiert?
	// - because Map and Landscape are handled differently in NET2 (Map.bmp vs Landscape.bmp), and the zoomed Map.bmp may be used
	//   to reconstruct the textures on the Landscape.bmp in case of e.g. runtime joins. In this sense, C4S.Landscape.ExactLandscape
	//   only marks that the landscape.bmp is an exact one, and there may or may not be an accompanying Map.bmp
	Landscape.MapZoom.Set(1,0,1,1);
	// Zoom player starting positions
	for (int32_t cnt=0; cnt<C4S_MaxPlayer; cnt++)
		{
		if (PlrStart[cnt].PositionX >= -1)
			PlrStart[cnt].PositionX = PlrStart[cnt].PositionX * iMapZoom;
		if (PlrStart[cnt].PositionY >= -1)
			PlrStart[cnt].PositionY = PlrStart[cnt].PositionY * iMapZoom;
		}
		*/
	}

bool C4SDefinitions::GetModules(StdStrBuf *psOutModules) const
	{
	// Local only
	if (LocalOnly) { psOutModules->Copy(""); return TRUE; }
	// Scan for any valid entries
	bool fSpecified = false;
	int32_t cnt=0;
	for (; cnt<C4S_MaxDefinitions; cnt++)
		if (Definition[cnt][0])
			fSpecified = true;
	// No valid entries
	if (!fSpecified) return false;
	// Compose entry list
	psOutModules->Copy("");
	for (cnt=0; cnt<C4S_MaxDefinitions; cnt++)
		if (Definition[cnt][0])
			{
			if (psOutModules->getLength()) psOutModules->AppendChar(';');
			psOutModules->Append(Definition[cnt]);
			}
	// Done
	return TRUE;
	}


void C4SDefinitions::SetModules(const char *szList, const char *szRelativeToPath, const char *szRelativeToPath2)
	{
	int32_t cnt;

	// Empty list: local only
	if (!SModuleCount(szList)) 
		{ 
		LocalOnly=TRUE; 
		for (cnt=0; cnt<C4S_MaxDefinitions; cnt++) Definition[cnt][0]=0;
		return;
		}

	// Set list
	LocalOnly=FALSE;
	for (cnt=0; cnt<C4S_MaxDefinitions; cnt++) 
		{
		SGetModule(szList,cnt,Definition[cnt],_MAX_PATH);
		// Make relative path
		if (szRelativeToPath && *szRelativeToPath)
			if (SEqualNoCase(Definition[cnt],szRelativeToPath,SLen(szRelativeToPath)))
				SCopy(Definition[cnt]+SLen(szRelativeToPath),Definition[cnt]);
		if (szRelativeToPath2 && *szRelativeToPath2)
			if (SEqualNoCase(Definition[cnt],szRelativeToPath2,SLen(szRelativeToPath2)))
				SCopy(Definition[cnt]+SLen(szRelativeToPath2),Definition[cnt]);
		}

	}

BOOL C4SDefinitions::AssertModules(const char *szPath, char *sMissing)
	{
	// Local only
	if (LocalOnly) return TRUE;

	// Check all listed modules for availability
	BOOL fAllAvailable=TRUE;
	char szModule[_MAX_PATH+1];
	if (sMissing) sMissing[0]=0;
	// Check all definition files
	for (int32_t cnt=0; cnt<C4S_MaxDefinitions; cnt++)
		if (Definition[cnt][0])
			{
			// Compose filename using path specified by caller
			szModule[0]=0;
			if (szPath) SCopy(szPath,szModule); if (szModule[0]) AppendBackslash(szModule);
			SAppend(Definition[cnt],szModule);
			// Missing
			if (!C4Group_IsGroup(szModule))
				{
				// Add to list
				if (sMissing) { SNewSegment(sMissing,", "); SAppend(Definition[cnt],sMissing); }
				fAllAvailable=FALSE;
				}
			}

	return fAllAvailable;
	}

void C4SDefinitions::CompileFunc(StdCompiler *pComp)
  {
  pComp->Value(mkNamingAdapt(LocalOnly,               "LocalOnly",             FALSE));
	pComp->Value(mkNamingAdapt(AllowUserChange,         "AllowUserChange",       FALSE));
  for(int32_t i = 0; i < C4S_MaxDefinitions; i++)
    pComp->Value(mkNamingAdapt(mkStringAdaptMA(Definition[i]), FormatString("Definition%i", i+1).getData(), ""));
  pComp->Value(mkNamingAdapt(SkipDefs,                "SkipDefs",              C4IDList()));
  }

void C4SGame::ConvertGoals(C4SRealism &rRealism)
	{
	
	// Convert mode to goals
	switch (Mode)
		{
		case C4S_Melee: Goals.SetIDCount(C4Id("MELE"),1,TRUE); ClearOldGoals(); break;
		case C4S_MeleeTeamwork: Goals.SetIDCount(C4Id("MELE"),1,TRUE); ClearOldGoals(); break;
		}
	Mode=0; 
	
	// Convert goals (master selection)
	switch (CooperativeGoal)
		{
		case C4S_Goldmine: Goals.SetIDCount(C4Id("GLDM"),1,TRUE); ClearOldGoals(); break;
		case C4S_Monsterkill: Goals.SetIDCount(C4Id("MNTK"),1,TRUE); ClearOldGoals(); break;
		case C4S_ValueGain: Goals.SetIDCount(C4Id("VALG"),Max(ValueGain/100,1),TRUE); ClearOldGoals(); break;
		}
	CooperativeGoal=0;
	// CreateObjects,ClearObjects,ClearMaterials are still valid but invisible

	// Convert realism to rules
	if (rRealism.ConstructionNeedsMaterial) Rules.SetIDCount(C4Id("CNMT"),1,TRUE); rRealism.ConstructionNeedsMaterial=0;
	if (rRealism.StructuresNeedEnergy) Rules.SetIDCount(C4Id("ENRG"),1,TRUE); rRealism.StructuresNeedEnergy=0;
	
	// Convert rules
	if (EnableRemoveFlag) Rules.SetIDCount(C4Id("FGRV"),1,TRUE); EnableRemoveFlag=0;

	// Convert eliminiation to rules
	switch (Elimination)
		{
		case C4S_KillTheCaptain: Rules.SetIDCount(C4Id("KILC"),1,TRUE); break;
		case C4S_CaptureTheFlag: Rules.SetIDCount(C4Id("CTFL"),1,TRUE); break;
		}
	Elimination=1; // unconvertible default crew elimination

	// CaptureTheFlag requires FlagRemoveable
	if (Rules.GetIDCount(C4Id("CTFL"))) Rules.SetIDCount(C4Id("FGRV"),1,TRUE);

	}

void C4SGame::ClearOldGoals()
	{
	CreateObjects.Clear(); ClearObjects.Clear(); ClearMaterial.Clear();
	ValueGain=0;
	}

BOOL C4SGame::IsMelee()
	{
	return (Goals.GetIDCount(C4Id("MELE")) || Goals.GetIDCount(C4Id("MEL2")));
	}



#ifdef C4ENGINE

// scenario sections

const char *C4ScenSect_Main = "main";

C4ScenarioSection::C4ScenarioSection(char *szName)
	{
	// copy name
	if (szName && !SEqualNoCase(szName, C4ScenSect_Main) && *szName)
		{
		this->szName = new char[strlen(szName)+1];
		SCopy(szName, this->szName);
		}
	else
		this->szName = const_cast<char *>(C4ScenSect_Main);
	// zero fields
	szTempFilename = szFilename = 0;
	fModified = false;
	// link into main list
	pNext = Game.pScenarioSections;
	Game.pScenarioSections = this;
	}

C4ScenarioSection::~C4ScenarioSection()
	{
	// del following scenario sections
	while (pNext)
		{
		C4ScenarioSection *pDel = pNext;
		pNext = pNext->pNext;
		pDel->pNext = NULL;
		delete pDel;
		}
	// del temp file
	if (szTempFilename)
		{
		EraseItem(szTempFilename);
		delete szTempFilename;
		}
	// del filename if assigned
	if (szFilename) delete szFilename;
	// del name if owned
	if (szName != C4ScenSect_Main) delete szName;
	}

bool C4ScenarioSection::ScenarioLoad(char *szFilename)
	{
	// safety
	if (this->szFilename || !szFilename) return false;
	// store name
	this->szFilename = new char[strlen(szFilename)+1];
	SCopy(szFilename, this->szFilename, _MAX_FNAME);
	// extract if it's not an open folder
	if (Game.ScenarioFile.IsPacked()) if (!EnsureTempStore(true, true)) return false;
	// donce, success
	return true;
	}

C4Group *C4ScenarioSection::GetGroupfile(C4Group &rGrp)
	{
	// check temp filename
	if (szTempFilename) if (rGrp.Open(szTempFilename)) return &rGrp; else return NULL;
	// check filename within scenario
	if (szFilename) if (rGrp.OpenAsChild(&Game.ScenarioFile, szFilename)) return &rGrp; else return NULL;
	// unmodified main section: return main group
	if (SEqualNoCase(szName, C4ScenSect_Main)) return &Game.ScenarioFile;
	// failure
	return NULL;
	}

bool C4ScenarioSection::EnsureTempStore(bool fExtractLandscape, bool fExtractObjects)
	{
	// if it's temp store already, don't do anything
	if (szTempFilename) return true;
	// make temp filename
	char *szTmp = const_cast<char *>(Config.AtTempPath(szFilename ? GetFilename(szFilename) : szName));
	MakeTempFilename(szTmp);
	// main section: extract section files from main scenario group (create group as open dir)
	if (!szFilename)
		{
		if (!CreateDirectory(szTmp, NULL)) return false;
		C4Group hGroup;
		if (!hGroup.Open(szTmp, TRUE)) { EraseItem(szTmp); return false; }
		// extract all desired section files
		Game.ScenarioFile.ResetSearch();
		char fn[_MAX_FNAME+1]; *fn=0;
		while (Game.ScenarioFile.FindNextEntry(C4FLS_Section, fn))
			if (fExtractLandscape || !WildcardMatch(C4FLS_SectionLandscape, fn))
				if (fExtractObjects || !WildcardMatch(C4FLS_SectionObjects, fn))
					Game.ScenarioFile.ExtractEntry(fn, szTmp);
		hGroup.Close();
		}
	else
		{
		// subsection: simply extract section from main group
		if (!Game.ScenarioFile.ExtractEntry(szFilename, szTmp)) return false;
		// delete undesired landscape/object files
		if (!fExtractLandscape || !fExtractObjects)
			{
			C4Group hGroup;
			if (hGroup.Open(szFilename))
				{
				if (!fExtractLandscape) hGroup.Delete(C4FLS_SectionLandscape);
				if (!fExtractObjects) hGroup.Delete(C4FLS_SectionObjects);
				}
			}
		}
	// copy temp filename
	szTempFilename = new char[strlen(szTmp)+1];
	SCopy(szTmp, szTempFilename, _MAX_PATH);
	// done, success
	return true;
	}

#endif // C4ENGINE
