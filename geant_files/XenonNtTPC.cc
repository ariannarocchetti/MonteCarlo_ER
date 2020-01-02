// XENON Header Files
#include "XenonNtTPC.hh"
#include "Xenon1tGridParameterisation.hh"
#include "Xenon1tLXeSensitiveDetector.hh"
#include "Xenon1tPMTsR11410.hh"
#include "Xenon1tPMTsR8520.hh"

// Additional Header Files
#include <globals.hh>

using std::stringstream;
using std::vector;

// G4 Header Files
#include <G4Cons.hh>
#include <G4IntersectionSolid.hh>
#include <G4LogicalBorderSurface.hh>
#include <G4Material.hh>
#include <G4PVParameterised.hh>
#include <G4PVPlacement.hh>
#include <G4SDManager.hh>
#include <G4Torus.hh>
#include <G4Trd.hh>
#include <G4UnionSolid.hh>
#if GEANTVERSION >= 10
#include <G4SystemOfUnits.hh>
#endif

// Class describing the TPC of XENONnT
// Diego Ramírez 27-11-2017

XenonNtTPC::XenonNtTPC(Xenon1tDetectorConstruction *det,
                       G4LogicalVolume *MotherLogicalVolume, G4bool pFlagHVFT,
                       G4bool pCathodeFlag, G4bool pBottomScreeningFlag,
                       G4bool pRealTopScreeningFlag, G4bool pRealAnodeFlag,
                       G4bool pRealMeshFlag, G4bool pRealS2Flag,
                       G4String pTopPMTPatternGeometry,
                       G4int m_iVerbosityLevel)
    : m_pMotherLogicalVolume(MotherLogicalVolume) {
  pFeedthroughFlag = pFlagHVFT;
  pRealCathodeMesh = pCathodeFlag;                  // not used
  pRealBottomScreeningMesh = pBottomScreeningFlag;  // not used
  pRealTopScreeningMesh = pRealTopScreeningFlag;    // not used
  pRealAnodeMesh = pRealAnodeFlag;                  // not used
  pRealGateMesh = pRealMeshFlag;                    // not used
  pRealS2Mesh = pRealS2Flag;                        // not used

  TopPMTPatternGeometry = pTopPMTPatternGeometry;
  iVerbosityLevel = m_iVerbosityLevel;

  DefineGeometryParametersNT(det);
  Materials = det->GetMaterials();
}

XenonNtTPC::~XenonNtTPC() { ; }

//======================= Geometry parameters XENONnT ========================
map<G4String, G4double> XenonNtTPC::m_hGeometryParametersNT;

void XenonNtTPC::DefineGeometryParametersNT(Xenon1tDetectorConstruction *det) {
  // Shrinkage coefficients (i.e., every dimension given here is warm!!)
  // For pillars and walls:
  m_hGeometryParametersNT["PTFE_ShrinkageZ"] = 0.014; //1,4 %
  // For PMT holders and reflector plates:
  m_hGeometryParametersNT["PTFE_ShrinkageR"] = 0.011; //1,1 %
  // For electrode frame holders:
  m_hGeometryParametersNT["Torlon_ShrinkageZ"] = 0.0025; //0,25 %

  // Bell
  m_hGeometryParametersNT["BellPlateHeight"] = 5. * mm;
  m_hGeometryParametersNT["BellPlateDiameter"] = 1415. * mm;
  m_hGeometryParametersNT["BellPlateTopToIVcylinderTop"] = 48.681 * mm;
  m_hGeometryParametersNT["BellPlateOffsetZ"] = 
    0.5 * det->GetGeometryParameter("InnerCryostatCylinderHeight")
    - GetGeometryParameterNT("BellPlateTopToIVcylinderTop")
    - 0.5 * GetGeometryParameterNT("BellPlateHeight");  
  m_hGeometryParametersNT["BellWallOuterDiameter"] = 1426. * mm;
  m_hGeometryParametersNT["BellWallHeight"] = 264. * mm;
  m_hGeometryParametersNT["BellWallThickness"] = 5. * mm;
  m_hGeometryParametersNT["BellWallBotToGateRingBot"] = 5. * mm;

  // Copper ring
  m_hGeometryParametersNT["CopperRingHeight"] = 10. * mm;
  m_hGeometryParametersNT["CopperRingInnerDiameter"] = 1364. * mm;

  // PMTs
  if (TopPMTPatternGeometry == "radial") {
    m_hGeometryParametersNT["NbOfTopPMTs"] = 225;
  } else if (TopPMTPatternGeometry == "hexagonal") {
    m_hGeometryParametersNT["NbOfTopPMTs"] = 253;
  }
  m_hGeometryParametersNT["NbOfBottomPMTs"] = 241;
  m_hGeometryParametersNT["NbOfPMTs"] =
      GetGeometryParameterNT("NbOfTopPMTs") +
      GetGeometryParameterNT("NbOfBottomPMTs");
  m_hGeometryParametersNT["PMTheight"] = 114. * mm;
  
  // Top PMTs assembly
  m_hGeometryParametersNT["TopPmtStemToBases"] = 8. * mm;
  m_hGeometryParametersNT["PmtBasesDiameter"] = 35. * mm;
  m_hGeometryParametersNT["PmtBasesHeight"] = 1.55 * mm;
  m_hGeometryParametersNT["TopBasesToBottomBellPlate"] = 37.39 * mm;

  m_hGeometryParametersNT["TopPTFEholderDiameter"] = 1370. * mm;
  m_hGeometryParametersNT["TopPTFEholderHeight"] = 5.1 * mm;
  m_hGeometryParametersNT["TopPTFEholderTopToPmtBaseBot"] = 2.15 * mm;
  m_hGeometryParametersNT["TopPTFEholderHoleDiameter"] = 39. * mm;

  m_hGeometryParametersNT["TopCopperPlateDiameter"] = 1412. * mm;
  m_hGeometryParametersNT["TopCopperPlateHeight"] = 20.0 * mm;
  m_hGeometryParametersNT["TopCopperPlateTopToPTFEholderBot"] = 29.9 * mm;
  m_hGeometryParametersNT["TopCopperPlateHoleDiameter"] = 79. * mm;

  m_hGeometryParametersNT["TopReflectorDiameter"] = 1412. * mm;
  m_hGeometryParametersNT["TopReflectorHeight"] = 8. * mm;
  m_hGeometryParametersNT["TopReflectorTopToCopperPlateBot"] = 60. * mm;
  m_hGeometryParametersNT["TopReflectorConeHoleDmax"] = 68. * mm;
  m_hGeometryParametersNT["TopReflectorConeHoleDmin"] = 64. * mm;
  m_hGeometryParametersNT["TopReflectorConeHoleHeight"] = 2. * mm;
  m_hGeometryParametersNT["TopReflectorTube1Height"] = 0.9 * mm;
  m_hGeometryParametersNT["TopReflectorTube2Diameter"] = 73.5 * mm;
  m_hGeometryParametersNT["TopReflectorTube2Height"] = 0.85 * mm;
  m_hGeometryParametersNT["TopReflectorTube3Diameter"] = 78.5 * mm;
  m_hGeometryParametersNT["TopReflectorTube3Height"] = 4.25 * mm;

  // Grids & rings
  m_hGeometryParametersNT["TopMeshRingHeight"] = 15. * mm;
  m_hGeometryParametersNT["TopMeshRingWidth"] = 31. * mm;
  m_hGeometryParametersNT["TopMeshRingInnerDiameter"] = 1334. * mm;
  m_hGeometryParametersNT["AnodeRingTopToTopMeshRingBot"] = 10. * mm;
  m_hGeometryParametersNT["TopMeshThickness"] = 0.216 * mm;
  m_hGeometryParametersNT["TopMeshDiameter"] = 1334. * mm;

  m_hGeometryParametersNT["AnodeRingHeight"] = 18. * mm;
  m_hGeometryParametersNT["AnodeRingWidth"] = 31. * mm;
  m_hGeometryParametersNT["AnodeRingInnerDiameter"] = 1334. * mm;
  m_hGeometryParametersNT["GateRingTopToAnodeRingBot"] = 8. * mm;
  m_hGeometryParametersNT["AnodeMeshDiameter"] = 1334. * mm;
  m_hGeometryParametersNT["AnodeMeshThickness"] = 0.216 * mm;

  m_hGeometryParametersNT["GateRingTotalHeight"] = 20. * mm;
  m_hGeometryParametersNT["GateRingTotalWidth"] = 31. * mm;
  m_hGeometryParametersNT["GateRingInnerDiameterMax"] = 1354. * mm;
  m_hGeometryParametersNT["GateRingInnerDiameterMin"] = 1334. * mm;
  m_hGeometryParametersNT["GateRingHeightSmallDiamRegion"] = 9. * mm;
  m_hGeometryParametersNT["GateMeshDiameter"] = 1334. * mm;
  m_hGeometryParametersNT["GateMeshThickness"] = 0.216 * mm;

  m_hGeometryParametersNT["TpcBotToCathodeRingTop"] = 0.8 * mm;
  m_hGeometryParametersNT["CathodeRingTubeWidth"] = 24. * mm;
  m_hGeometryParametersNT["CathodeRingTubeHeight"] = 10. * mm;
  m_hGeometryParametersNT["CathodeRingTotalHeight"] = 20. * mm;
  m_hGeometryParametersNT["CathodeRingTorusRadius"] = 10. * mm;
  m_hGeometryParametersNT["CathodeRingInnerDiameter"] = 1347. * mm;
  m_hGeometryParametersNT["CathodeMeshDiameter"] = 1347. * mm;
  m_hGeometryParametersNT["CathodeMeshThickness"] = 0.300 * mm;

  m_hGeometryParametersNT["BMringTopToCathodeRingBot"] = 20.195 * mm;
  m_hGeometryParametersNT["BMringTubeWidth"] = 25. * mm;
  m_hGeometryParametersNT["BMringTubeHeight"] = 7.5 * mm;
  m_hGeometryParametersNT["BMringTotalHeight"] = 15. * mm;
  m_hGeometryParametersNT["BMringTorusRadius"] = 7.5 * mm;  
  m_hGeometryParametersNT["BMringInnerDiameter"] = 1345. * mm;
  m_hGeometryParametersNT["BottomMeshDiameter"] = 1345. * mm;
  m_hGeometryParametersNT["BottomMeshThickness"] = 0.216 * mm;

  m_hGeometryParametersNT["RingBelowGateHeight"] = 5 * mm;
  m_hGeometryParametersNT["RingBelowGateInnerDiameter"] = GetGeometryParameterNT("CopperRingInnerDiameter");
  m_hGeometryParametersNT["RingBelowGateWidth"] = 25.5 * mm;

  // TPC
  m_hGeometryParametersNT["TpcWallDiameter"] = 1328. * mm; // Panel to panel
  m_hGeometryParametersNT["TpcWallHeight"] = 1500.8 * mm;
  m_hGeometryParametersNT["TpcWallThickness"] = 3. * mm;
  m_hGeometryParametersNT["TopGateRingToTopTPC"] = 0.8 * mm;

  // Frame electrode rings top
  m_hGeometryParametersNT["ThinElectrodesFrameHeight"] = 14.9 * mm;
  m_hGeometryParametersNT["ThinElectrodesFrameWidth"] = 3. * mm;
  m_hGeometryParametersNT["ElectrodesFrameWidth"] = 36.255 * mm;
  m_hGeometryParametersNT["FrameTopMeshHeight"] = 18.3 * mm;
  m_hGeometryParametersNT["FrameTopMeshToAnodeHeight"] = 1.2 * mm;
  m_hGeometryParametersNT["FrameAnodeHeight"] = 26.8 * mm;
  m_hGeometryParametersNT["FrameAnodeToGateHeight"] = 1.2 * mm;
  m_hGeometryParametersNT["FrameGateHeight"] = 27.5 * mm;
  m_hGeometryParametersNT["ElectrodesFrameHeight"] =
     GetGeometryParameterNT("FrameTopMeshHeight")
     + GetGeometryParameterNT("FrameTopMeshToAnodeHeight")
     + GetGeometryParameterNT("FrameAnodeHeight")
     + GetGeometryParameterNT("FrameAnodeToGateHeight")
     + GetGeometryParameterNT("FrameGateHeight");
  m_hGeometryParametersNT["ElectrodesFrameHeightAboveTMRing"] = 4. * mm;
  m_hGeometryParametersNT["ElectrodesFrameBetweenRingsInnerR"] =
     0.5 * GetGeometryParameterNT("TpcWallDiameter") + 11.154 * mm;
  m_hGeometryParametersNT["ElectrodesFrameAboveGateRingsInnerR"] =
     0.5 * GetGeometryParameterNT("TpcWallDiameter") + 6.525 * mm;
  m_hGeometryParametersNT["FrameInletAboveGateRingHeight"] = 0.5 * mm;
  m_hGeometryParametersNT["FrameGasFeedthroughRadius"] = 5.08 * mm;
  m_hGeometryParametersNT["FrameGasFeedthroughToTopFrame"] = 9.4 * mm;

  // Field rings and guards
  m_hGeometryParametersNT["NumerOfFieldShaperWires"] = 72;
  m_hGeometryParametersNT["FieldShaperWireDiameter"] = 2. * mm;
  m_hGeometryParametersNT["FieldShaperWiresDistance"] = 22. * mm;
  m_hGeometryParametersNT["FieldShaperWireTopToTpcTop"] = 10.2 * mm;

  m_hGeometryParametersNT["NumerOfFieldGuards"] = 64;
  m_hGeometryParametersNT["FieldGuardsHeight"] = 15. * mm;
  m_hGeometryParametersNT["FieldGuardsTubeHeight"] = 10. * mm;
  m_hGeometryParametersNT["FieldGuardsWidth"] = 5. * mm;
  m_hGeometryParametersNT["FieldGuardsDistance"] = 22. * mm;
  m_hGeometryParametersNT["TopGuardToTopFieldShaperWire"] = 59.5 * mm;
  m_hGeometryParametersNT["GuardToFRSradialDistance"] = 9.16 * mm;

  // PTFE pillars
  m_hGeometryParametersNT["NumberOfPillars"] = 24;
  m_hGeometryParametersNT["PillarsDeltaTheta"] =
     360. * deg / GetGeometryParameterNT("NumberOfPillars");
  m_hGeometryParametersNT["BottomBoxBase_x"] = 18. * mm;
  m_hGeometryParametersNT["BottomBoxBase_y"] = 18. * mm;
  m_hGeometryParametersNT["BottomBox_height"] = 130.875 * mm;
  m_hGeometryParametersNT["Trapezoid_y1"] = 36. * mm;
  m_hGeometryParametersNT["Trapezoid_y2"] = 49. * mm;
  m_hGeometryParametersNT["Trapezoid_height"] = 13. * mm;
  m_hGeometryParametersNT["MiddleBoxBase_y"] = 36. * mm;
  m_hGeometryParametersNT["MiddleBox_height"] = 1466.91 * mm;
  m_hGeometryParametersNT["MiddleBox_height_UpToTrapezoid"] = 1436.63 * mm;
  m_hGeometryParametersNT["TopBoxBase_y"] = //19. * mm;
     0.5 * GetGeometryParameterNT("CopperRingInnerDiameter")
     - 0.5 * GetGeometryParameterNT("TpcWallDiameter")
     - GetGeometryParameterNT("TpcWallThickness");
  m_hGeometryParametersNT["TopBox_height"] = 15. * mm;

  // Copper ring below pillars
  m_hGeometryParametersNT["CuBelowPillarsInnerDiameter"] = 1400. * mm;
  m_hGeometryParametersNT["CuBelowPillarsWidth"] = 10. * mm;
  m_hGeometryParametersNT["CuBelowPillarsHeight"] = 10. * mm;

  // Bottom TPC
  m_hGeometryParametersNT["BottomTpcTopToTpcBot"] = 1.507 * mm;
  m_hGeometryParametersNT["BottomTpcHeight"] = 53.78 * mm;
  m_hGeometryParametersNT["BottomTpcWidth"] = 3. * mm;

  // (24 x) PTFE frame above cathode ring
  m_hGeometryParametersNT["PTFEAboveCathodeHeight"] = 15. * mm;
  m_hGeometryParametersNT["PTFEAboveCathodeTopInnerR"] = 685.5 * mm;
  m_hGeometryParametersNT["PTFEAboveCathodeTopWidth"] = 2. * mm;
  m_hGeometryParametersNT["PTFEAboveCathodeBotInnerR"] = 697.5 * mm;
  m_hGeometryParametersNT["PTFEAboveCathodeBotWidth"] = 2. * mm;
  m_hGeometryParametersNT["PTFEAboveCathodeMiddleHeight"] = 3. * mm;
  m_hGeometryParametersNT["PTFEAboveCathodeBotHeight"] = 5. * mm;
  m_hGeometryParametersNT["PTFECathodeAngularSeparation"] = 0.02793;

  // Bottom PTFE ring below BM ring
  m_hGeometryParametersNT["TeflonBMringWidth"] = 2. * mm;
  m_hGeometryParametersNT["TeflonBMringInnerD"] = 1380.5 * mm;

  // Bottom PMTs assembly
  m_hGeometryParametersNT["BMringBotToPTFEReflectorTop"] = 3.905 * mm;
  m_hGeometryParametersNT["BotReflectorTopToPMTtop"] = 3.15 * mm;
  m_hGeometryParametersNT["BotReflectorDiameter"] = 1395. * mm;

  m_hGeometryParametersNT["BotCopperPlateDiameter"] = 1420. * mm;
  m_hGeometryParametersNT["BotCopperPlateHeight"] = 25. * mm;
  m_hGeometryParametersNT["BotPTFEReflectorToBotCopperPlateTop"] = 56. * mm;
  m_hGeometryParametersNT["BotCopperPlateHoleDiameter"] = 79. * mm;

  m_hGeometryParametersNT["BotCopperPlateToTopPTFEholder"] = 28.9 * mm;
       
  m_hGeometryParametersNT["GateRingTopToGXeInterface"] = 
     0.5 * (1 - GetGeometryParameterNT("PTFE_ShrinkageZ")) 
         * GetGeometryParameterNT("GateRingTopToAnodeRingBot");
}

G4double XenonNtTPC::GetGeometryParameterNT(const char *szParameterNT) {
  if (m_hGeometryParametersNT.find(szParameterNT) !=
      m_hGeometryParametersNT.end()) {
    return m_hGeometryParametersNT[szParameterNT];
  } else {
    G4cout << "----> Parameter " << szParameterNT << " is not defined!!!!!"
           << G4endl;
    return 0;
  }
}

//============================== TPC construction=============================
G4VPhysicalVolume *XenonNtTPC::ConstructTPC(Xenon1tDetectorConstruction *det) {
  ConstructXenon(det);

  // if(pFeedthroughFlag)  ConstructHVFT(det);

  Xenon1tPMTsR11410 *pPMTR11410 = new Xenon1tPMTsR11410(det);
  m_pPmtR11410LogicalVolume = pPMTR11410->Construct();

  if (iVerbosityLevel >= 1)
    G4cout << "XenonNtTPC::Construct() TopTPC " << G4endl;

  ConstructTopTPC();

  //_____ xenon sensitivity _____
  G4SDManager *pSDManager = G4SDManager::GetSDMpointer();
  Xenon1tLXeSensitiveDetector *pLXeSD =
      new Xenon1tLXeSensitiveDetector("Xenon1t/LXeSD");
  pSDManager->AddNewDetector(pLXeSD);
  m_pLXeLogicalVolume->SetSensitiveDetector(pLXeSD);
  m_pGXeLogicalVolume->SetSensitiveDetector(pLXeSD);

  if (iVerbosityLevel >= 1)
    G4cout << "XenonNtTPC::Construct() TPC " << G4endl;
  ConstructMainTPC();
  ConstructGrids();

  // Make ConstructXenon actually pass logical volume to a physical volume
  return m_pLXePhysicalVolume;
}

void XenonNtTPC::ConstructXenon(Xenon1tDetectorConstruction *det) {
  if (iVerbosityLevel >= 1)
    G4cout << "Xenon1tDetectorConstruction::ConstructXenon() Construct Xenon "
           << G4endl;

  G4Material *LXe = G4Material::GetMaterial("LXe");
  G4Material *GXe = G4Material::GetMaterial("GXe");

  //G4double dTorlonCorrZ = 1 - GetGeometryParameterNT("Torlon_ShrinkageZ");
  G4double dPTFECorrZ = 1 - GetGeometryParameterNT("PTFE_ShrinkageZ");

  G4double dLength = det->GetGeometryParameter("InnerCryostatCylinderHeight");
  G4double dD = det->GetGeometryParameter("InnerCryostatOuterDiameter") -
                2 * det->GetGeometryParameter("InnerCryostatThickness");
  G4double dR0top = det->GetGeometryParameter("InnerCryostatR0top") -
                    det->GetGeometryParameter("InnerCryostatThicknessTop");
  G4double dR1top = det->GetGeometryParameter("InnerCryostatR1top") -
                    det->GetGeometryParameter("InnerCryostatThicknessTop");
  G4double dR0bot = det->GetGeometryParameter("InnerCryostatR0bot") -
                    det->GetGeometryParameter("InnerCryostatThicknessBot");
  G4double dR1bot = det->GetGeometryParameter("InnerCryostatR1bot") -
                    det->GetGeometryParameter("InnerCryostatThicknessBot");

  //__________ Liquid Xenon __________

  // DR 20181002 - Unlike for the 1T TPC, full inner cryostat is filled with LXe.
  //               This approach is followed in order to place the bell and other
  //               top TPC components (contained in LXe and GXe) correctly.
  G4UnionSolid *pXeUnionSolid = det->ConstructVessel(
      dD, dLength, dR0top, dR1top, dR0bot, dR1bot, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, true, true);
  m_pLXeLogicalVolume = new G4LogicalVolume(
      pXeUnionSolid, LXe, "XenonLogicalVolume", 0, 0, 0, true);
  // Logical volume name intentionally ambiguous. It contains LXe and GXe.
  m_pLXePhysicalVolume =
      new G4PVPlacement(0, G4ThreeVector(0, 0, 0), m_pLXeLogicalVolume,
                        "LXe", m_pMotherLogicalVolume, false, 0);

  //__________ Gaseous Xenon __________

  // DR 20181002 - Subtraction solid of full inner vessel ('pXeUnionSolid') minus
  //               bell, minus electrode rings frame, minus LXe.

  // Construct Bell volumes for subtraction
  G4double dBellPlateHeight = GetGeometryParameterNT("BellPlateHeight");
  G4double dBellPlateRadius = 0.5 * GetGeometryParameterNT("BellPlateDiameter");
  G4double dBellPlateOffsetZ = GetGeometryParameterNT("BellPlateOffsetZ");
  G4double dBellWallHeight = GetGeometryParameterNT("BellWallHeight");
  G4double dBellWallOuterRadius =
             0.5 * GetGeometryParameterNT("BellWallOuterDiameter");
  G4double dBellWallInnerRadius =
             dBellWallOuterRadius - GetGeometryParameterNT("BellWallThickness");            
  G4double dBellWallOffsetZ = dBellPlateOffsetZ + 0.5 * dBellPlateHeight
                              - 0.5 * dBellWallHeight;

  G4Tubs *pCut1 = new G4Tubs("pCutBellPlate", 0. * cm, dBellPlateRadius,
                             dBellPlateHeight * 0.5, 0., 2 * M_PI);
  G4Tubs *pCut2 = new G4Tubs("pCutBellWall", dBellWallInnerRadius,
                             dBellWallOuterRadius, dBellWallHeight * 0.5,
                             0., 2 * M_PI);

  // Construct rings frame for subtraction
  G4double dHeightThinFrame =
      dPTFECorrZ * GetGeometryParameterNT("ThinElectrodesFrameHeight");
  G4double dHeightFrame =
      dPTFECorrZ * GetGeometryParameterNT("ElectrodesFrameHeight")
      + dHeightThinFrame;

  G4double dCuRingHeight = GetGeometryParameterNT("CopperRingHeight");
  G4double dCuRingOffsetZ = dBellWallOffsetZ - 0.5 * dBellWallHeight
                            - 0.5 * dCuRingHeight;
  G4double dGateRingTotalHeight = GetGeometryParameterNT("GateRingTotalHeight");
  G4double dGateRingOffsetZ = dCuRingOffsetZ + 0.5 * dCuRingHeight
            + dPTFECorrZ * GetGeometryParameterNT("BellWallBotToGateRingBot")
            + 0.5 * dGateRingTotalHeight;

  G4SubtractionSolid *pElectrodesFrame = ConstructTopRingsFrame();

  G4double zPosFrameOffsetZ = dGateRingOffsetZ - 0.5 * dGateRingTotalHeight
      + (1 - dPTFECorrZ) * dGateRingTotalHeight
      + 0.5 * dHeightFrame;

  // Construct LXe volume for subtraction
  G4double dGXeCutShiftZ = dGateRingOffsetZ + 0.5 * dGateRingTotalHeight
     - det->GetGeometryParameter("InnerCryostatCylinderHeight")
     + GetGeometryParameterNT("GateRingTopToGXeInterface");
  
  G4Tubs *pCut3 = new G4Tubs("pCutLXe", 0., dD, dLength, 0., 2 * M_PI);

  // Subtract Bell Plate
  G4SubtractionSolid *pGXe = 
               new G4SubtractionSolid("GXe", pXeUnionSolid, pCut1, 0,
                                      G4ThreeVector(0., 0., dBellPlateOffsetZ));

  // Subtract Bell Wall
  pGXe = new G4SubtractionSolid("GXe", pGXe, pCut2, 0,
                                G4ThreeVector(0., 0., dBellWallOffsetZ));

  // Subtract LXe level
  pGXe = new G4SubtractionSolid("GXe", pGXe, pCut3, 0,
                                G4ThreeVector(0., 0., dGXeCutShiftZ));

  // Subtract torlon frame
  pGXe = new G4SubtractionSolid("GXe", pGXe, pElectrodesFrame, 0,
                                G4ThreeVector(0., 0., zPosFrameOffsetZ));

  m_pGXeLogicalVolume =
      new G4LogicalVolume(pGXe, GXe, "GXeLogicalVolume", 0, 0, 0);
  m_pGXePhysicalVolume =
      new G4PVPlacement(0, G4ThreeVector(0, 0, 0), m_pGXeLogicalVolume, "GXe",
                        m_pLXeLogicalVolume, false, 0);

  //==== attributes ====
  G4Colour hLXeColor(0.094, 0.718, 0.812, 0.05);
  G4VisAttributes *pLXeVisAtt = new G4VisAttributes(hLXeColor);
  pLXeVisAtt->SetVisibility(false);
  m_pLXeLogicalVolume->SetVisAttributes(pLXeVisAtt);

  G4Colour hGXeColor(0.539, 0.318, 0.378, 0.01);
  G4VisAttributes *pGXeVisAtt = new G4VisAttributes(hGXeColor);
  pGXeVisAtt->SetVisibility(false);
  m_pGXeLogicalVolume->SetVisAttributes(pGXeVisAtt);
}

void XenonNtTPC::ConstructTopTPC() {
  G4Material *SS316Ti = G4Material::GetMaterial("SS316Ti");
  G4Material *Copper = G4Material::GetMaterial("Copper");
  G4Material *Cirlex = G4Material::GetMaterial("Cirlex");
  G4Material *Teflon = G4Material::GetMaterial("Teflon");
  //G4Material *Torlon = G4Material::GetMaterial("Torlon");

  //G4double dTorlonCorrZ = 1 - GetGeometryParameterNT("Torlon_ShrinkageZ");
  G4double dPTFECorrZ = 1 - GetGeometryParameterNT("PTFE_ShrinkageZ");
  G4double dPTFECorrR = 1 - GetGeometryParameterNT("PTFE_ShrinkageR");

  //__________ Bell __________

  G4double dBellPlateHeight = GetGeometryParameterNT("BellPlateHeight");
  G4double dBellPlateRadius = 0.5 * GetGeometryParameterNT("BellPlateDiameter");
  G4double dBellPlateOffsetZ = GetGeometryParameterNT("BellPlateOffsetZ");
  G4double dBellWallHeight = GetGeometryParameterNT("BellWallHeight");
  G4double dBellWallOuterRadius =
             0.5 * GetGeometryParameterNT("BellWallOuterDiameter");
  G4double dBellWallInnerRadius =
             dBellWallOuterRadius - GetGeometryParameterNT("BellWallThickness");            
  G4double dBellWallOffsetZ = dBellPlateOffsetZ + 0.5 * dBellPlateHeight
                              - 0.5 * dBellWallHeight;

  G4Tubs *pBellPlate = new G4Tubs("BellPlateTube", 0. * cm, dBellPlateRadius,
                                  dBellPlateHeight * 0.5, 0., 2 * M_PI);
  m_pBellPlateLogicalVolume = new G4LogicalVolume(pBellPlate, SS316Ti,
                                                  "BellPlateLogicalVolume",
                                                  0, 0, 0);
  m_pBellPlatePhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(0., 0., dBellPlateOffsetZ), m_pBellPlateLogicalVolume,
      "SS_BellPlate", m_pLXeLogicalVolume, false, 0);

  G4Tubs *pBellWall =
          new G4Tubs("BellWallTube", dBellWallInnerRadius, dBellWallOuterRadius,
                     dBellWallHeight * 0.5, 0., 2 * M_PI);
  m_pBellWallLogicalVolume = new G4LogicalVolume(
      pBellWall, SS316Ti, "BellWallLogicalVolume", 0, 0, 0);
  m_pBellWallPhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(0., 0., dBellWallOffsetZ), m_pBellWallLogicalVolume,
      "SS_BellSideWall", m_pLXeLogicalVolume, false, 0);

  //__________ Copper ring below bell __________

  G4double dCuRingHeight = GetGeometryParameterNT("CopperRingHeight");
  G4double dCuRingInnerRadius =
           0.5 * GetGeometryParameterNT("CopperRingInnerDiameter");
  G4double dCuRingOffsetZ = dBellWallOffsetZ - 0.5 * dBellWallHeight
                           - 0.5 * dCuRingHeight;

  G4Tubs *pCuRing = new G4Tubs("CopperRingTube", dCuRingInnerRadius + 0.2 * mm,
                               dBellWallInnerRadius,
                               0.5 * dCuRingHeight - 0.2 * mm, 0., 2 * M_PI);
  m_pCuRingLogicalVolume = new G4LogicalVolume(pCuRing, Copper,
                                               "CopperRingLogicalVolume",
                                               0, 0, 0);
  m_pCuRingPhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(0., 0., dCuRingOffsetZ), m_pCuRingLogicalVolume,
      "Copper_TopRing", m_pLXeLogicalVolume, false, 0);

  //__________ PTFE ring below gate frame __________

  G4double dPTFEringBelowGateHeight =
            dPTFECorrZ * GetGeometryParameterNT("RingBelowGateHeight");
  G4double dPTFEringBelowGateInnerR =
            0.5 * GetGeometryParameterNT("RingBelowGateInnerDiameter")
            + 0.2 * mm; // To account for the straight shape of pillars
  G4double dPTFEringBelowGateWidth =
            GetGeometryParameterNT("RingBelowGateWidth");
  G4double dPTFEringBelowGateOffsetZ =
            dCuRingOffsetZ + 0.5 * dCuRingHeight
            + 0.5 * dPTFEringBelowGateHeight;

  G4Tubs *pPTFEring =
            new G4Tubs("PTFEringTube", dPTFEringBelowGateInnerR,
                       dPTFEringBelowGateInnerR + dPTFEringBelowGateWidth,
                       0.5 * dPTFEringBelowGateHeight, 0., 2 * M_PI);
  m_pTopTeflonRingLogicalVolume = new G4LogicalVolume(pPTFEring, Teflon,
                                               "TeflonRingLogicalVolume",
                                               0, 0, 0);
  m_pTopTeflonRingPhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(0., 0., dPTFEringBelowGateOffsetZ),
      m_pTopTeflonRingLogicalVolume, "Teflon_RingBelowGate",
      m_pLXeLogicalVolume, false, 0);

  new G4LogicalBorderSurface("PTFEringBelowGateLogicalBorderSurface",
                             m_pLXePhysicalVolume, 
                             m_pTopTeflonRingPhysicalVolume,
                             Materials->LXeTeflonOpticalSurface());

  //__________ Bases Top PMTs __________

  G4double dPmtBasesRadius = GetGeometryParameterNT("PmtBasesDiameter") * 0.5;
  G4double dPmtBasesHeight = GetGeometryParameterNT("PmtBasesHeight");
  G4double dTopPmtBasesOffsetZ =
            dBellPlateOffsetZ - 0.5 * (dBellPlateHeight + dPmtBasesHeight)
            - GetGeometryParameterNT("TopBasesToBottomBellPlate");

  G4Tubs *pPmtBases = new G4Tubs("pPmtBases", 0., dPmtBasesRadius,
                                 dPmtBasesHeight * 0.5, 0, 2 * M_PI);
  m_pPmtBasesLogicalVolume =
      new G4LogicalVolume(pPmtBases, Cirlex, "PmtBasesLogicalVolume", 0, 0, 0);

  //__________ Top PMTs (R11410) __________

  G4int TotNbOfTopPMTs = G4int(GetGeometryParameterNT("NbOfTopPMTs"));
  G4double dPMTsOffsetZ = 
            dTopPmtBasesOffsetZ - GetGeometryParameterNT("TopPmtStemToBases")
            - 0.5 * (dPmtBasesHeight + GetGeometryParameterNT("PMTheight"));

  //__________ Top PMTs holder (PTFE) __________

  G4double dTopPMTholderRadius =
            GetGeometryParameterNT("TopPTFEholderDiameter") * 0.5;
  G4double dTopPMTholderHeight =
            GetGeometryParameterNT("TopPTFEholderHeight");
  G4double dTopPmtHolderOffsetZ =
            dTopPmtBasesOffsetZ - 0.5 * (dPmtBasesHeight + dTopPMTholderHeight)
            - GetGeometryParameterNT("TopPTFEholderTopToPmtBaseBot");

  G4Tubs *pTopPmtHolderTube =
      new G4Tubs("TopPmtHolderTube", 0., dTopPMTholderRadius,
                 dTopPMTholderHeight * 0.5, 0, 2 * M_PI);

  // Hole for subtraction during PMTs placement
  G4double dTopPMTholderHoleRadius =
        dPTFECorrR * GetGeometryParameterNT("TopPTFEholderHoleDiameter") * 0.5;

  G4Tubs *pTopPmtHolderCut =
      new G4Tubs("TopPmtHolderCut", 0., dTopPMTholderHoleRadius,
                 dTopPMTholderHeight * 0.5, 0, 2 * M_PI);

  //__________ Top copper plate __________

  G4double dTopCopperPlateRadius =
                GetGeometryParameterNT("TopCopperPlateDiameter") * 0.5;
  G4double dTopCopperPlateHeight = GetGeometryParameterNT("TopCopperPlateHeight");
  G4double dTopCopperPlateOffsetZ =
              dTopPmtHolderOffsetZ
              - 0.5 * (dTopPMTholderHeight + dTopCopperPlateHeight)
              - GetGeometryParameterNT("TopCopperPlateTopToPTFEholderBot");

  G4Tubs *pTopCopperPlateTube =
      new G4Tubs("TopCopperPlateTube", 0., dTopCopperPlateRadius,
                 dTopCopperPlateHeight * 0.5, 0, 2 * M_PI);

  // Hole for subtraction during PMTs placement
  G4double dTopCopperPlateHoleRadius =
                 GetGeometryParameterNT("TopCopperPlateHoleDiameter") * 0.5;

  G4Tubs *pTopCopperPlateCut =
      new G4Tubs("TopCopperPlateCut", 0., dTopCopperPlateHoleRadius,
                 dTopCopperPlateHeight * 0.5, 0, 2 * M_PI);

  //__________ Top PTFE reflector __________

  G4double dTopReflectorRadius =
               GetGeometryParameterNT("TopReflectorDiameter") * 0.5;
  G4double dTopReflectorHeight = GetGeometryParameterNT("TopReflectorHeight");
  G4double dTopReflectorOffsetZ =
              dTopCopperPlateOffsetZ
              - 0.5 * (dTopCopperPlateHeight + dTopReflectorHeight)
              - GetGeometryParameterNT("TopReflectorTopToCopperPlateBot");

  G4Tubs *pTopReflectorTube =
      new G4Tubs("TopReflectorTube", 0., dTopReflectorRadius,
                 dTopReflectorHeight * 0.5, 0, 2 * M_PI);

  // Here the four holes are merged, so there is only one subtraction later

  // The one going through the whole plate goes first (2nd one from the cone)
  G4double dTopReflectorHoleRadius =
        dPTFECorrR * GetGeometryParameterNT("TopReflectorConeHoleDmin") * 0.5;
  G4Tubs *pTopReflectorHoleBase =
      new G4Tubs("TopReflectorHole", 0., dTopReflectorHoleRadius,
                 dTopReflectorHeight * 0.5, 0, 2 * M_PI);
  
  // Merging conical hole  
  G4double dHoleConeRmax =
        dPTFECorrR * GetGeometryParameterNT("TopReflectorConeHoleDmax") * 0.5;
  G4double dHoleConeHeight=
               GetGeometryParameterNT("TopReflectorConeHoleHeight");
  G4Cons *pCutHoleCone =
     new G4Cons("CutHoleCone", 0., dHoleConeRmax, 0., dTopReflectorHoleRadius,
                dHoleConeHeight * 0.5, 0., 2 * M_PI);

  G4double zPosOverlap = 0.5 * (dHoleConeHeight - dTopReflectorHeight);
  G4UnionSolid *pTopReflectorCut =
      new G4UnionSolid("TopReflectorHole_2", pTopReflectorHoleBase, pCutHoleCone,
                       0, G4ThreeVector(0., 0., zPosOverlap));

  // Merging 3rd hold from cone (closest to window)
  G4double dHole3Radius =
        dPTFECorrR * GetGeometryParameterNT("TopReflectorTube2Diameter") * 0.5;
  G4double dHole3Height=
               GetGeometryParameterNT("TopReflectorTube2Height");
  G4Tubs *pCutHoleTube3 =
      new G4Tubs("CutHoleTube3", 0., dHole3Radius,
                 dHole3Height * 0.5, 0, 2 * M_PI);

  G4double dHole2Height = GetGeometryParameterNT("TopReflectorTube1Height");
  zPosOverlap = zPosOverlap + 0.5 * (dHoleConeHeight + dHole3Height)
                + dHole2Height;
  pTopReflectorCut =
      new G4UnionSolid("TopReflectorHole_3", pTopReflectorCut, pCutHoleTube3,
                       0, G4ThreeVector(0., 0., zPosOverlap));

  // Merging 4rd hold from cone
  G4double dHole4Radius =
        dPTFECorrR * GetGeometryParameterNT("TopReflectorTube3Diameter") * 0.5;
  G4double dHole4Height=
               GetGeometryParameterNT("TopReflectorTube3Height");
  G4Tubs *pCutHoleTube4 =
      new G4Tubs("CutHoleTube4", 0., dHole4Radius,
                 dHole4Height * 0.5, 0, 2 * M_PI);

  zPosOverlap = 0.5 * (dTopReflectorHeight - dHole4Height);
  pTopReflectorCut =
      new G4UnionSolid("TopReflectorCut", pTopReflectorCut, pCutHoleTube4,
                       0, G4ThreeVector(0., 0., zPosOverlap));

  //__________ Placement of Top PMT Array and Cuts for Plates __________

  // Placeholders for the holes subtraction
  G4SubtractionSolid *pTopPmtHolder;
  G4SubtractionSolid *pTopCopperPlate;
  G4SubtractionSolid *pTopReflector;

  stringstream hVolumeName;
  stringstream hVolumeName_bases;
  stringstream hHoleName_reflector;
  stringstream hHoleName_copper;
  stringstream hHoleName_ptfeholder;

  for (G4int iPMTNt = 0; iPMTNt < TotNbOfTopPMTs; ++iPMTNt) {
    G4ThreeVector PmtPosition;

    if (TopPMTPatternGeometry == "radial") {
      PmtPosition = GetPMTsPositionTopArray_rad(iPMTNt);
    } else if (TopPMTPatternGeometry == "hexagonal") {
      PmtPosition = GetPMTsPositionTopArray_hex(iPMTNt);
    }

    // Placing PMTs and bases
    hVolumeName.str("");
    hVolumeName << "PmtTpcTop_" << iPMTNt;
    m_pPMTPhysicalVolumes.push_back(
        new G4PVPlacement(0, PmtPosition + G4ThreeVector(0., 0., dPMTsOffsetZ),
                          m_pPmtR11410LogicalVolume, hVolumeName.str(),
                          m_pGXeLogicalVolume, false, iPMTNt));

    hVolumeName_bases.str("");
    hVolumeName_bases << "PmtBaseTpcTop_" << iPMTNt;
    m_pPmtBasesPhysicalVolumes.push_back(new G4PVPlacement(
        0, PmtPosition + G4ThreeVector(0., 0., dTopPmtBasesOffsetZ),
        m_pPmtBasesLogicalVolume, hVolumeName_bases.str(), m_pGXeLogicalVolume,
        false, iPMTNt));

    // Time to subtract holes at the PMT positions

    if (iPMTNt==0) {
      hHoleName_ptfeholder.str("");
      hHoleName_ptfeholder << "TopPTFEholderWithCuts_" << iPMTNt;
      pTopPmtHolder =
           new G4SubtractionSolid(hHoleName_ptfeholder.str(), pTopPmtHolderTube,
                                  pTopPmtHolderCut, 0, PmtPosition);

      hHoleName_copper.str("");
      hHoleName_copper << "TopCopperWithCuts_" << iPMTNt;
      pTopCopperPlate =
            new G4SubtractionSolid(hHoleName_copper.str(), pTopCopperPlateTube,
                                   pTopCopperPlateCut, 0, PmtPosition);

      hHoleName_reflector.str("");
      hHoleName_reflector << "TopReflectorWithCuts_" << iPMTNt;
      pTopReflector =
            new G4SubtractionSolid(hHoleName_reflector.str(), pTopReflectorTube,
                                   pTopReflectorCut, 0, PmtPosition);
    }    
    else {
      hHoleName_ptfeholder.str("");
      hHoleName_ptfeholder << "TopPTFEholderWithCuts_" << iPMTNt;
      pTopPmtHolder =
           new G4SubtractionSolid(hHoleName_ptfeholder.str(), pTopPmtHolder,
                                  pTopPmtHolderCut, 0, PmtPosition);

      hHoleName_copper.str("");
      hHoleName_copper << "TopCopperWithCuts_" << iPMTNt;
      pTopCopperPlate =
            new G4SubtractionSolid(hHoleName_copper.str(), pTopCopperPlate,
                                   pTopCopperPlateCut, 0, PmtPosition);

      hHoleName_reflector.str("");
      hHoleName_reflector << "TopReflectorWithCuts_" << iPMTNt;
      pTopReflector =
            new G4SubtractionSolid(hHoleName_reflector.str(), pTopReflector,
                                   pTopReflectorCut, 0, PmtPosition);
    }        
  }

  m_pTopPMTHolderLogicalVolume = new G4LogicalVolume(pTopPmtHolder, Teflon,
                                                     "TopPmtHolderLogicalVolume",
                                                     0, 0, 0);
  m_pTopPMTHolderPhysicalVolume =
      new G4PVPlacement(0, G4ThreeVector(0., 0., dTopPmtHolderOffsetZ),
                        m_pTopPMTHolderLogicalVolume, "Teflon_TopPmtHolder",
                        m_pGXeLogicalVolume, false, 0);
                        
  new G4LogicalBorderSurface("TopPmtHolderLogicalBorderSurface",
                             m_pGXePhysicalVolume, 
                             m_pTopPMTHolderPhysicalVolume,
                             Materials->GXeTeflonOpticalSurface());

  m_pTopPMTCopperLogicalVolume = new G4LogicalVolume(pTopCopperPlate, Copper,
                                                     "TopCopperPlateLogicalVolume",
                                                     0, 0, 0);
  m_pTopPMTCopperPhysicalVolume =
      new G4PVPlacement(0, G4ThreeVector(0., 0., dTopCopperPlateOffsetZ),
                        m_pTopPMTCopperLogicalVolume, "Copper_TopPmtPlate",
                        m_pGXeLogicalVolume, false, 0);

  m_pTopPMTReflectorLogicalVolume =
      new G4LogicalVolume(pTopReflector, Teflon,
                            "TopReflectorLogicalVolume", 0, 0, 0);
  m_pTopPMTReflectorPhysicalVolume =
      new G4PVPlacement(0, G4ThreeVector(0., 0., dTopReflectorOffsetZ),
                        m_pTopPMTReflectorLogicalVolume, "Teflon_TopReflector",
                        m_pGXeLogicalVolume, false, 0);

  new G4LogicalBorderSurface("TopReflectorLogicalBorderSurface",
                             m_pGXePhysicalVolume, 
                             m_pTopPMTReflectorPhysicalVolume,
                             Materials->GXeTeflonOpticalSurface());
                        
  //__________ Gate ring __________

  G4double dGateRingTotalHeight = GetGeometryParameterNT("GateRingTotalHeight");
  G4double dGateRingTotalWidth = GetGeometryParameterNT("GateRingTotalWidth");  
  G4double dGateRingBottomInnerRadius =
        0.5 * GetGeometryParameterNT("GateRingInnerDiameterMax");
  G4double dGateRingTopInnerRadius =
        0.5 * GetGeometryParameterNT("GateRingInnerDiameterMin");
  G4double dGateRingTopHeight =
        GetGeometryParameterNT("GateRingHeightSmallDiamRegion");
  G4double dGateRingBottomHeight =
        dGateRingTotalHeight - dGateRingTopHeight;
  G4double dGateRingOffsetZ = dCuRingOffsetZ + 0.5 * dCuRingHeight
            + dPTFECorrZ * GetGeometryParameterNT("BellWallBotToGateRingBot")
            + 0.5 * dGateRingTotalHeight;
         
  G4Tubs *pGateRingTube =
         new G4Tubs("GateRingTube", dGateRingTopInnerRadius,
                    dGateRingTopInnerRadius + dGateRingTotalWidth,
                    0.5 * dGateRingTotalHeight,
                    0., 2 * M_PI);
  G4Tubs *pGateRingCut =
         new G4Tubs("GateRingCut", 0, dGateRingBottomInnerRadius,
                    0.5 * dGateRingBottomHeight, 0., 2 * M_PI);

  G4SubtractionSolid *pGateRing = new G4SubtractionSolid(
    "GateRing", pGateRingTube, pGateRingCut, 0, G4ThreeVector(
    0., 0., 0.5 * (dGateRingBottomHeight - dGateRingTotalHeight)));
  m_pGateRingLogicalVolume = new G4LogicalVolume(
      pGateRing, SS316Ti, "GateRingLogicalVolume", 0, 0, 0);
  m_pGateRingPhysicalVolume =
      new G4PVPlacement(0, G4ThreeVector(0., 0., dGateRingOffsetZ),
                        m_pGateRingLogicalVolume, "SS_GateRing",
                        m_pLXeLogicalVolume, false, 0);

  //__________ Anode ring __________

  G4double dAnodeRingHeight = GetGeometryParameterNT("AnodeRingHeight");
  G4double dAnodeRingWidth = GetGeometryParameterNT("AnodeRingWidth");
  G4double dAnodeRingInnerR =
        0.5 * GetGeometryParameterNT("AnodeRingInnerDiameter");
  G4double dAnodeRingOffsetZ = dGateRingOffsetZ + 0.5 * dGateRingTotalHeight
        + dPTFECorrZ * GetGeometryParameterNT("GateRingTopToAnodeRingBot")
        + 0.5 * dAnodeRingHeight;

  G4Tubs *pAnodeRing = new G4Tubs("AnodeRing", dAnodeRingInnerR,
                                  dAnodeRingInnerR + dAnodeRingWidth,
                                  0.5 * dAnodeRingHeight, 0., 2 * M_PI);
  m_pAnodeRingLogicalVolume = new G4LogicalVolume(
      pAnodeRing, SS316Ti, "AnodeRingLogicalVolume", 0, 0, 0);
  m_pAnodeRingPhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(0., 0., dAnodeRingOffsetZ), m_pAnodeRingLogicalVolume,
      "SS_AnodeRing", m_pGXeLogicalVolume, false, 0);

  //__________ Top Mesh ring __________

  G4double dTopMeshRingHeight = GetGeometryParameterNT("TopMeshRingHeight");
  G4double dTopMeshRingWidth = GetGeometryParameterNT("TopMeshRingWidth");
  G4double dTopMeshRingInnerR =
        0.5 * GetGeometryParameterNT("TopMeshRingInnerDiameter");
  G4double dTopMeshRingOffsetZ = dAnodeRingOffsetZ + 0.5 * dAnodeRingHeight
        + dPTFECorrZ * GetGeometryParameterNT("AnodeRingTopToTopMeshRingBot")
        + 0.5 * dTopMeshRingHeight;

  G4Tubs *pTopMeshRing = new G4Tubs("TopMeshRing", dTopMeshRingInnerR,
                                    dTopMeshRingInnerR + dTopMeshRingWidth,
                                    0.5 * dTopMeshRingHeight, 0., 2 * M_PI);
  m_pTopMeshRingLogicalVolume = new G4LogicalVolume(
      pTopMeshRing, SS316Ti, "TopMeshRingLogicalVolume", 0, 0, 0);
  m_pTopMeshRingPhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(0., 0., dTopMeshRingOffsetZ), m_pTopMeshRingLogicalVolume,
      "SS_TopMeshRing", m_pGXeLogicalVolume, false, 0);

  //__________ Frame for electrode rings __________

  G4double dHeightThinFrame =
      dPTFECorrZ * GetGeometryParameterNT("ThinElectrodesFrameHeight");
  G4double dHeightFrame =
      dPTFECorrZ * GetGeometryParameterNT("ElectrodesFrameHeight")
      + dHeightThinFrame;
      
  G4SubtractionSolid *pElectrodesFrame = ConstructTopRingsFrame();
  G4double zPosFrameOffsetZ = dGateRingOffsetZ - 0.5 * dGateRingTotalHeight
      + (1 - dPTFECorrZ) * dGateRingTotalHeight
      + 0.5 * dHeightFrame;

  G4double dHeightLXeFrame = GetGeometryParameterNT("GateRingTopToGXeInterface")
      + dGateRingTotalHeight - (1 - dPTFECorrZ) * dGateRingTotalHeight; 

  G4double dFrameOuterR = GetGeometryParameterNT("TpcWallDiameter")
                          + GetGeometryParameterNT("ElectrodesFrameWidth");
      
  // Construct GXeTeflon frame by subtracting the LXe part
  G4Tubs *subtract_lxeteflon =
      new G4Tubs("subtraction_lxeteflon", 0, dFrameOuterR,
                  0.5 * dHeightLXeFrame, 0., 2 * M_PI);
  G4SubtractionSolid *pElectrodesFrame_gxeteflon =
      new G4SubtractionSolid("ElectrodesFrame_gxeteflon", pElectrodesFrame,
                              subtract_lxeteflon, 0, 
                              G4ThreeVector(0., 0., - 0.5 * dHeightFrame + 0.5 * dHeightLXeFrame));
  
  // Construct LXeTeflon frame by subtracting the GXe part
  G4Tubs *subtract_gxeteflon =
      new G4Tubs("subtraction_gxeteflon", 0, dFrameOuterR,
                 0.5 * (dHeightFrame - dHeightLXeFrame), 0., 2 * M_PI);
  G4SubtractionSolid *pElectrodesFrame_lxeteflon =
      new G4SubtractionSolid("ElectrodesFrame_lxeteflon", pElectrodesFrame,
                             subtract_gxeteflon, 0, G4ThreeVector(0., 0., 0.5 * dHeightLXeFrame));
  
  m_pTopElectrodesFrameGXeTeflonLogicalVolume = new G4LogicalVolume(
     pElectrodesFrame_gxeteflon, Teflon, "ElectrodesFrameGXeTeflonLogicalVolume",
     0, 0, 0);
  m_pTopElectrodesFrameGXeTeflonPhysicalVolume = new G4PVPlacement(
     0, G4ThreeVector(0., 0., zPosFrameOffsetZ),
     m_pTopElectrodesFrameGXeTeflonLogicalVolume, "GXeTeflon_TopElectrodesFrame",
     m_pLXeLogicalVolume, false, 0);
     
  new G4LogicalBorderSurface("ElectrodesFrameGXeTeflonLogicalBorderSurface",
                             m_pGXePhysicalVolume, 
                             m_pTopElectrodesFrameGXeTeflonPhysicalVolume,
                             Materials->GXeTeflonUnpolishedOpticalSurface());

  m_pTopElectrodesFrameLXeTeflonLogicalVolume = new G4LogicalVolume(
    pElectrodesFrame_lxeteflon, Teflon, "ElectrodesFrameTeflonLogicalVolume",
    0, 0, 0);
  m_pTopElectrodesFrameLXeTeflonPhysicalVolume = new G4PVPlacement(
     0, G4ThreeVector(0., 0., zPosFrameOffsetZ),
     m_pTopElectrodesFrameLXeTeflonLogicalVolume, "Teflon_TopElectrodesFrame",
     m_pLXeLogicalVolume, false, 0);

  new G4LogicalBorderSurface("ElectrodesFrameLXeTeflonLogicalBorderSurface",
                             m_pLXePhysicalVolume, 
                             m_pTopElectrodesFrameGXeTeflonPhysicalVolume,
                             Materials->LXeTeflonUnpolishedOpticalSurface());

  //==== attributes ====
  G4Colour hCopperColor(1., 0.757, 0.24, 0.1);
  G4VisAttributes *pCopperVisAtt = new G4VisAttributes(hCopperColor);
  pCopperVisAtt->SetVisibility(true);
  m_pCuRingLogicalVolume->SetVisAttributes(pCopperVisAtt);
  
  G4Colour hPMTCopperColor(1., 0.757, 0.24, 0.1);
  G4VisAttributes *pPMTCopperVisAtt = new G4VisAttributes(hPMTCopperColor);
  pPMTCopperVisAtt->SetVisibility(false);
  m_pTopPMTCopperLogicalVolume->SetVisAttributes(pPMTCopperVisAtt);

  G4Colour hCirlexColor(0.2, 0.5, 0.8, 0.1);
  G4VisAttributes *pCirlexVisAtt = new G4VisAttributes(hCirlexColor);
  pCirlexVisAtt->SetVisibility(true);
  m_pPmtBasesLogicalVolume->SetVisAttributes(pCirlexVisAtt);

  G4Colour hGXeTeflonColor(0.6, 0.4, 0.3, 0.02);
  G4VisAttributes *pGXeTeflonVisAtt = new G4VisAttributes(hGXeTeflonColor);
  pGXeTeflonVisAtt->SetVisibility(true);
  m_pTopTeflonRingLogicalVolume->SetVisAttributes(pGXeTeflonVisAtt);
  pGXeTeflonVisAtt->SetVisibility(false);
  m_pTopElectrodesFrameGXeTeflonLogicalVolume->SetVisAttributes(pGXeTeflonVisAtt);
  m_pTopElectrodesFrameLXeTeflonLogicalVolume->SetVisAttributes(pGXeTeflonVisAtt);
  
  G4Colour hPMTGXeTeflonColor(0.6, 0.4, 0.3, 0.02);
  G4VisAttributes *pPMTGXeTeflonVisAtt = new G4VisAttributes(hPMTGXeTeflonColor);
  pPMTGXeTeflonVisAtt->SetVisibility(true);
  m_pTopPMTHolderLogicalVolume->SetVisAttributes(pPMTGXeTeflonVisAtt);
  pPMTGXeTeflonVisAtt->SetVisibility(false);
  m_pTopPMTReflectorLogicalVolume->SetVisAttributes(pPMTGXeTeflonVisAtt);
  
  G4Colour hSSColor(0.600, 0.600, 0.600, 0.1);
  G4VisAttributes *pSSVisAtt = new G4VisAttributes(hSSColor);
  pSSVisAtt->SetVisibility(true);
  m_pGateRingLogicalVolume->SetVisAttributes(pSSVisAtt);
  m_pAnodeRingLogicalVolume->SetVisAttributes(pSSVisAtt);
  m_pTopMeshRingLogicalVolume->SetVisAttributes(pSSVisAtt);
}

void XenonNtTPC::ConstructMainTPC() {
  G4Material *Teflon = G4Material::GetMaterial("Teflon");
  G4Material *Copper = G4Material::GetMaterial("Copper");
  G4Material *Cirlex = G4Material::GetMaterial("Cirlex");
  G4Material *SS316Ti = G4Material::GetMaterial("SS316Ti");

  G4double dPTFECorrZ = 1 - GetGeometryParameterNT("PTFE_ShrinkageZ");
  G4double dPTFECorrR = 1 - GetGeometryParameterNT("PTFE_ShrinkageR");

  G4RotationMatrix *pRotX180 = new G4RotationMatrix();
  pRotX180->rotateX(180. * deg);

  // DR 20181018 - Like in the case of the GXe cut, the offsets from the
  //               relevant 'ConstructTopTPC' components are propagated
  //               for convenience.

  //~~~~~~~
    // Bell
    G4double dBellPlateHeight = GetGeometryParameterNT("BellPlateHeight");
    G4double dBellPlateOffsetZ = GetGeometryParameterNT("BellPlateOffsetZ");
    G4double dBellWallHeight = GetGeometryParameterNT("BellWallHeight");
    G4double dBellWallOffsetZ = dBellPlateOffsetZ + 0.5 * dBellPlateHeight
                              - 0.5 * dBellWallHeight;
    // Copper ring below bell
    G4double dCuRingHeight = GetGeometryParameterNT("CopperRingHeight");
    G4double dCuRingOffsetZ = dBellWallOffsetZ - 0.5 * dBellWallHeight
                           - 0.5 * dCuRingHeight;
    // Gate ring
    G4double dGateRingTotalHeight = GetGeometryParameterNT("GateRingTotalHeight");
    G4double dGateRingOffsetZ = dCuRingOffsetZ + 0.5 * dCuRingHeight
            + dPTFECorrZ * GetGeometryParameterNT("BellWallBotToGateRingBot")
            + 0.5 * dGateRingTotalHeight;
  //~~~~~~~

  //__________ TPC wall __________

  G4double dTpcInRadius = 0.5 * GetGeometryParameterNT("TpcWallDiameter");
  G4double dTpcOutRadius =
      dTpcInRadius + GetGeometryParameterNT("TpcWallThickness");
  G4double dTpcHeight = dPTFECorrZ * GetGeometryParameterNT("TpcWallHeight");
  G4double dTpcOffsetZ = dGateRingOffsetZ + 0.5 * dGateRingTotalHeight
      - GetGeometryParameterNT("TopGateRingToTopTPC")
      - 0.5 * dTpcHeight;

  if (iVerbosityLevel >= 1)
    G4cout << "=== TPC wall (PTFE) ===\n"
           << "  Inner R = " << dTpcInRadius << G4endl
           << "  Outer R = " << dTpcOutRadius << G4endl
           << "  Height (cold) = " << dTpcHeight << G4endl
           << "  Z offset = " << dTpcOffsetZ << G4endl;

  G4Tubs *pTpc = new G4Tubs("pTpc", dTpcInRadius, dTpcOutRadius,
                            dTpcHeight * 0.5, 0, 2 * M_PI);
  m_pTpcLogicalVolume =
      new G4LogicalVolume(pTpc, Teflon, "TPCWallLogicalVolume", 0, 0, 0);
  m_pTpcPhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(0., 0., dTpcOffsetZ), m_pTpcLogicalVolume,
      "Teflon_TPC", m_pLXeLogicalVolume, false, 0);
      
  new G4LogicalBorderSurface("TpcLogicalBorderSurface", 
                             m_pLXePhysicalVolume,
                             m_pTpcPhysicalVolume,
                             Materials->LXeTeflonOpticalSurface());

  //__________ Field shaper rings (wires) __________

  G4int iNumberOfFSRwires = GetGeometryParameterNT("NumerOfFieldShaperWires");
  G4double dFSRradius = 0.5 * GetGeometryParameterNT("FieldShaperWireDiameter");
  G4double dFSRcurvature = dTpcOutRadius + dFSRradius;
  G4double dFSRdistance = GetGeometryParameterNT("FieldShaperWiresDistance");
  G4double dFSRtopOffsetZ = dTpcOffsetZ + 0.5 * dTpcHeight
        - dPTFECorrZ * GetGeometryParameterNT("FieldShaperWireTopToTpcTop")
        - dFSRradius;

  G4Torus *pFSR =
      new G4Torus("FSRing", 0., dFSRradius, dFSRcurvature, 0, 2 * M_PI);
  m_pFieldShaperRingLogicalVolume = new G4LogicalVolume(
      pFSR, Copper, "FieldShaperRingLogicalVolume", 0, 0, 0);

  stringstream fsrname;
  G4double dFSROffsetZ = dFSRtopOffsetZ;

  if (iVerbosityLevel >= 1) {
    G4cout << "=== Field shaper rings ===\n"
           << "  Number = " << iNumberOfFSRwires << G4endl
           << "  Radius = " << dFSRradius << G4endl
           << "  (R,Z offset) of top wire (cold) = (" << dFSRcurvature
           << ", " << dFSRtopOffsetZ << ")" << G4endl;
  }

  // DR20181023 - Before constructing the copper rings, we call for the pillars
  //              construction, so that the first can be subtracted 
  G4UnionSolid *pPTFEpillar_0 = ConstructPillar();
  G4double dFirstFsrCutZ =
       dPTFECorrZ * GetGeometryParameterNT("MiddleBox_height") * 0.5
       + dPTFECorrZ * GetGeometryParameterNT("TopBox_height")
       + GetGeometryParameterNT("GateRingTotalHeight")
       - GetGeometryParameterNT("TopGateRingToTopTPC")
       - dPTFECorrZ * GetGeometryParameterNT("FieldShaperWireTopToTpcTop")
       - dPTFECorrZ * 0.5 * dFSRdistance
       - dFSRradius;
  G4double dFsrCutY = - 0.5 * GetGeometryParameterNT("MiddleBoxBase_y")
                      - dTpcOutRadius;
  G4SubtractionSolid *pPTFEpillar;
  stringstream holefsr_pillar;
  G4double dFsrCutZ = dFirstFsrCutZ;

  for (int i = 0; i < iNumberOfFSRwires; ++i) {
    fsrname.str("");
    fsrname << "Copper_FieldShaperRing_" << i;
    m_pFieldShaperRingPhysicalVolumes.push_back(new G4PVPlacement(
        0, G4ThreeVector(0., 0., dFSROffsetZ), m_pFieldShaperRingLogicalVolume,
        fsrname.str(), m_pLXeLogicalVolume, false, 0));
    if (i<5 || i>(iNumberOfFSRwires-4))
      dFSROffsetZ -= dPTFECorrZ * 0.5 * dFSRdistance;
    else
      dFSROffsetZ -= dPTFECorrZ * dFSRdistance;

    holefsr_pillar.str("");
    holefsr_pillar << "PTFEPillarWithFrsCuts_" << i; 

    if (i>0) { // First ring isn't in contact with the pillars
      if (i==1) {
        pPTFEpillar =
          new G4SubtractionSolid(holefsr_pillar.str(), pPTFEpillar_0,
                                 pFSR, 0,
                                 G4ThreeVector(0., dFsrCutY, dFsrCutZ));
      }
      else {      
        pPTFEpillar =
          new G4SubtractionSolid(holefsr_pillar.str(), pPTFEpillar,
                               pFSR, 0,
                               G4ThreeVector(0., dFsrCutY, dFsrCutZ));
      }
      
      if (i<5 || i>(iNumberOfFSRwires-4))
        dFsrCutZ -= dPTFECorrZ * 0.5 * dFSRdistance;
      else
        dFsrCutZ -= dPTFECorrZ * dFSRdistance;
    }
  }

  //__________ Field guards __________

  G4int iNumberOfGuards = GetGeometryParameterNT("NumerOfFieldGuards");
  G4double dGuardHeight = GetGeometryParameterNT("FieldGuardsHeight");
  G4double dGuardWidth = GetGeometryParameterNT("FieldGuardsWidth");
  G4double dGuardTubeHeight = GetGeometryParameterNT("FieldGuardsTubeHeight");
  G4double dGuardCurvature = dFSRcurvature + dFSRradius
        + GetGeometryParameterNT("GuardToFRSradialDistance")
        + 0.5 * dGuardWidth;
  G4double dGuardsDistance = GetGeometryParameterNT("FieldGuardsDistance");
  G4double dGuardTopOffsetZ = dFSRtopOffsetZ + dFSRradius
        - dPTFECorrZ * GetGeometryParameterNT("TopGuardToTopFieldShaperWire")
        - 0.5 * dGuardHeight;

  G4Torus *pGuardEdges =
      new G4Torus("GuardEdges", 0., 0.5 * (dGuardHeight - dGuardTubeHeight),
                  dGuardCurvature, 0, 2 * M_PI);
  G4Tubs *pGuardTube =
      new G4Tubs("GuarTube", dGuardCurvature - 0.5 * dGuardWidth,
                 dGuardCurvature + 0.5 * dGuardWidth,
                 0.5 * dGuardTubeHeight, 0, 2 * M_PI);

  G4UnionSolid *pFieldGuard =
      new G4UnionSolid("FieldGuard", pGuardTube, pGuardEdges, 0,
                       G4ThreeVector(0., 0., 0.5 * dGuardTubeHeight));
  pFieldGuard =
      new G4UnionSolid("FieldGuard", pFieldGuard, pGuardEdges, 0,
                       G4ThreeVector(0., 0., -0.5 * dGuardTubeHeight));
  m_pFieldGuardLogicalVolume = new G4LogicalVolume(
      pFieldGuard, Copper, "FieldGuardLogicalVolume", 0, 0, 0);

  stringstream guardname;
  G4double dGuardOffsetZ = dGuardTopOffsetZ;

  if (iVerbosityLevel >= 1) {
    G4cout << "=== Field guards ===\n"
           << "  Number = " << iNumberOfGuards << G4endl
           << "  Height = " << dGuardHeight << G4endl
           << "  Width = " << dGuardWidth << G4endl
           << "  (R,Z offset) of top guard (cold) = (" << dGuardCurvature
           << ", " << dGuardTopOffsetZ << ")" << G4endl;
  }

  G4double dFirstGuardCutZ = dFirstFsrCutZ + 
        + dPTFECorrZ * 0.5 * dFSRdistance
        + dFSRradius
        - dPTFECorrZ * GetGeometryParameterNT("TopGuardToTopFieldShaperWire")
        - 0.5 * dGuardHeight;
  G4double dGuardCutY = dFsrCutY;
  stringstream holeguards_pillar;
  G4double dGuardCutZ = dFirstGuardCutZ;

  for (int i = 0; i < iNumberOfGuards; ++i) {
    guardname.str("");
    guardname << "Copper_FieldGuard_" << i;
    m_pFieldGuardPhysicalVolumes.push_back(new G4PVPlacement(
        0, G4ThreeVector(0., 0., dGuardOffsetZ), m_pFieldGuardLogicalVolume,
        guardname.str(), m_pLXeLogicalVolume, false, 0));
    dGuardOffsetZ -= dPTFECorrZ * dGuardsDistance;

    holeguards_pillar.str("");
    holeguards_pillar << "PTFEPillarWithGuardCuts_" << i;
    pPTFEpillar =
        new G4SubtractionSolid(holeguards_pillar.str(), pPTFEpillar,
                               pFieldGuard, 0,
                               G4ThreeVector(0., dGuardCutY, dGuardCutZ));
    dGuardCutZ -= dPTFECorrZ * dGuardsDistance;
  }

  //__________ Pillars arrangement (constructed with rings) __________

  G4int dNbofPTFEpillar = GetGeometryParameterNT("NumberOfPillars");
  G4double dPTFEpillarCenterRadius =
       dTpcInRadius + GetGeometryParameterNT("TpcWallThickness")
       + GetGeometryParameterNT("MiddleBoxBase_y") * 0.5;
  G4double dPTFEpillarOffsetZ =
       dGateRingOffsetZ - 0.5 * dGateRingTotalHeight
       - dPTFECorrZ * GetGeometryParameterNT("TopBox_height")
       - dPTFECorrZ * GetGeometryParameterNT("MiddleBox_height") * 0.5;

  stringstream pillarname;
  G4double dPTFEpillarAngularStep = GetGeometryParameterNT("PillarsDeltaTheta");
  G4double dAngularOffset = 0.;

  m_pPTFEpillarLogicalVolume = new G4LogicalVolume(
        pPTFEpillar, Teflon, "PTFEpillarLogicalVolume", 0, 0, 0);

  for (int i = 0; i < dNbofPTFEpillar; i++) {
      G4RotationMatrix *zRot = new G4RotationMatrix();
      zRot->rotateZ(dPTFEpillarAngularStep * i);

      G4double PTFEpillarCenter_x = dPTFEpillarCenterRadius * sin(dAngularOffset);
      G4double PTFEpillarCenter_y = dPTFEpillarCenterRadius * cos(dAngularOffset);

      pillarname.str("");
      pillarname << "Teflon_Pillar_" << i;
      m_pPTFEpillarPhysicalVolumes.push_back(
        new G4PVPlacement(zRot,
                          G4ThreeVector(PTFEpillarCenter_x, PTFEpillarCenter_y,
                                        dPTFEpillarOffsetZ),
                          m_pPTFEpillarLogicalVolume, pillarname.str(),
                          m_pLXeLogicalVolume, false, 0));

      pillarname.str("");
      pillarname << "Teflon_PillarLogicalBorderSurface_" << i;
      G4VPhysicalVolume *PTFEPillarPhysicalVolume =
        (G4VPhysicalVolume *) m_pPTFEpillarPhysicalVolumes.at(i);
      new G4LogicalBorderSurface(pillarname.str(), 
                                 m_pLXePhysicalVolume,
                                 PTFEPillarPhysicalVolume,
                                 Materials->LXeTeflonOpticalSurface());
                          
      dAngularOffset += dPTFEpillarAngularStep;
  }

  //__________ Bottom TPC __________

  G4double dBottomTpcInRadius = dTpcInRadius;
  G4double dBottomTpcWidth = GetGeometryParameterNT("BottomTpcWidth");
  G4double dBottomTpcHeight =
             dPTFECorrZ * GetGeometryParameterNT("BottomTpcHeight");
  G4double dBottomTpcOffsetZ = dTpcOffsetZ - 0.5 * dTpcHeight
             - dPTFECorrZ * GetGeometryParameterNT("BottomTpcTopToTpcBot")
             - 0.5 * dBottomTpcHeight;

  G4Tubs *pBottomTpc = new G4Tubs("BottomTpc", dBottomTpcInRadius,
                                  dBottomTpcInRadius + dBottomTpcWidth,
                                  0.5 * dBottomTpcHeight, 0, 2 * M_PI);
  m_pBottomTpcLogicalVolume = new G4LogicalVolume(
      pBottomTpc, Teflon, "BottomTpcLogicalVolume", 0, 0, 0);
  m_pBottomTpcPhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(0., 0., dBottomTpcOffsetZ), m_pBottomTpcLogicalVolume,
      "Teflon_BottomTPC", m_pLXeLogicalVolume, false, 0);

  new G4LogicalBorderSurface("BottomPTFERingLogicalBorderSurface",
                             m_pLXePhysicalVolume, 
                             m_pBottomTpcPhysicalVolume,
                             Materials->LXeTeflonOpticalSurface());

  //__________ Cathode ring __________

  G4double dCathodeRingHeight = GetGeometryParameterNT("CathodeRingTotalHeight");
  G4double dCathodeRingWidth = GetGeometryParameterNT("CathodeRingTubeWidth");
  G4double dCathodeRingTorusR = GetGeometryParameterNT("CathodeRingTorusRadius");
  G4double dCathodeRingTubeHeight =
            GetGeometryParameterNT("CathodeRingTubeHeight");
  G4double dCathodeRingCurvature =
            0.5 * GetGeometryParameterNT("CathodeRingInnerDiameter");
  G4double dCathodeRingOffsetZ = dTpcOffsetZ - 0.5 * dTpcHeight
            - dPTFECorrZ * GetGeometryParameterNT("TpcBotToCathodeRingTop")
            - 0.5 * dCathodeRingTubeHeight;

  G4Tubs *pRingTube =
            new G4Tubs("CathodeRingTube", dCathodeRingCurvature,
                       dCathodeRingCurvature + dCathodeRingWidth,
                       0.5 * dCathodeRingTubeHeight, 0, 2 * M_PI);
  G4Tubs *pRingTubeBot =
    new G4Tubs("CathodeRingTube", dCathodeRingCurvature + dCathodeRingTorusR,
               dCathodeRingCurvature + dCathodeRingWidth - dCathodeRingTorusR,
               0.5 * (dCathodeRingHeight - dCathodeRingTubeHeight), 0, 2 * M_PI);
  G4Torus *pRingTorus1 =
      new G4Torus("CathodeRingTorus1", 0., dCathodeRingTorusR,
                  dCathodeRingCurvature + dCathodeRingTorusR, 0, 2 * M_PI);
  G4Torus *pRingTorus2 =
      new G4Torus("CathodeRingTorus2", 0., dCathodeRingTorusR,
                  dCathodeRingCurvature + dCathodeRingWidth - dCathodeRingTorusR,
                  0, 2 * M_PI);

  G4UnionSolid *pCathodeRing =
      new G4UnionSolid("CathodeRing", pRingTube, pRingTubeBot, 0,
                       G4ThreeVector(0., 0., -dCathodeRingTubeHeight));
  pCathodeRing =
      new G4UnionSolid("CathodeRing", pCathodeRing, pRingTorus1, 0,
                       G4ThreeVector(0., 0., -0.5 * dCathodeRingTubeHeight));
  pCathodeRing =
      new G4UnionSolid("CathodeRing", pCathodeRing, pRingTorus2, 0,
                       G4ThreeVector(0., 0., -0.5 * dCathodeRingTubeHeight));
  m_pCathodeMeshRingLogicalVolume = new G4LogicalVolume(
      pCathodeRing, SS316Ti, "CathodeRingLogicalVolume", 0, 0, 0);
  m_pCathodeMeshRingPhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(0., 0., dCathodeRingOffsetZ),
      m_pCathodeMeshRingLogicalVolume, "SS_CathodeRing", m_pLXeLogicalVolume,
      false, 0);

  //__________ (24 x) PTFE piece piece on top of cathode ring __________

  G4double dPTFECathodeHeight =
           GetGeometryParameterNT("PTFEAboveCathodeHeight");
  G4double dPTFECathodeTopInnerR =
           GetGeometryParameterNT("PTFEAboveCathodeTopInnerR");
  G4double dPTFECathodeTopWidth =
           GetGeometryParameterNT("PTFEAboveCathodeTopWidth");
  G4double dPTFECathodeBotInnerR =
           GetGeometryParameterNT("PTFEAboveCathodeBotInnerR");
  G4double dPTFECathodeBotWidth =
           GetGeometryParameterNT("PTFEAboveCathodeBotWidth");
  G4double dPTFECathodeMiddleHeight =
           GetGeometryParameterNT("PTFEAboveCathodeMiddleHeight");
  G4double dPTFECathodeBotHeight =
           GetGeometryParameterNT("PTFEAboveCathodeBotHeight");
  G4double dPTFECathodeSeparation =
           GetGeometryParameterNT("PTFECathodeAngularSeparation");
  G4double dPTFECathodeOffsetZ = dCathodeRingOffsetZ
                                 + 0.5 * dCathodeRingTubeHeight
                                 - dPTFECathodeBotHeight
                                 + 0.5 * dPTFECathodeHeight;

  const G4double dPTFECathodeZ0 = -0.5 * dPTFECathodeHeight;
  const G4double dPTFECathodeZ1 =
           dPTFECathodeZ0 + dPTFECathodeBotHeight;
  const G4double dPTFECathodeZ2 = dPTFECathodeZ1;
  const G4double dPTFECathodeZ3 =
           dPTFECathodeZ2 + dPTFECathodeMiddleHeight;
  const G4double dPTFECathodeZ4 = dPTFECathodeZ3;
  const G4double dPTFECathodeZ5 = 0.5 * dPTFECathodeHeight;

  const G4double dPTFECathodeInner0 = dPTFECathodeBotInnerR;
  const G4double dPTFECathodeInner1 = dPTFECathodeInner0;
  const G4double dPTFECathodeInner2 = dPTFECathodeTopInnerR;
  const G4double dPTFECathodeInner3 = dPTFECathodeInner2;
  const G4double dPTFECathodeInner4 = dPTFECathodeInner3;
  const G4double dPTFECathodeInner5 = dPTFECathodeInner4;

  const G4double dPTFECathodeOuter0 =
           dPTFECathodeInner0 + dPTFECathodeBotWidth;
  const G4double dPTFECathodeOuter1 = dPTFECathodeOuter0;
  const G4double dPTFECathodeOuter2 = dPTFECathodeOuter1;
  const G4double dPTFECathodeOuter3 = dPTFECathodeOuter2;
  const G4double dPTFECathodeOuter4 =
           dPTFECathodeInner4 + dPTFECathodeTopWidth;
  const G4double dPTFECathodeOuter5 = dPTFECathodeOuter4;

  const G4double dPTFECathodePlaneZ[] =
        {dPTFECathodeZ0, dPTFECathodeZ1, dPTFECathodeZ2,
         dPTFECathodeZ3, dPTFECathodeZ4, dPTFECathodeZ5};
  const G4double dPTFECathodeInner[] =
        {dPTFECathodeInner0, dPTFECathodeInner1, dPTFECathodeInner2,
         dPTFECathodeInner3, dPTFECathodeInner4, dPTFECathodeInner5};
  const G4double dPTFECathodeOuter[] =
        {dPTFECathodeOuter0, dPTFECathodeOuter1, dPTFECathodeOuter2,
         dPTFECathodeOuter3, dPTFECathodeOuter4, dPTFECathodeOuter5};

  G4Polycone *pPTFEcathode =
   new G4Polycone("PTFEcathode", 0.5 * dPTFECathodeSeparation,
                  (2. * M_PI)/24 - dPTFECathodeSeparation, 6,
                  dPTFECathodePlaneZ, dPTFECathodeInner, dPTFECathodeOuter);
  m_pCathodeRingTopFrameLogicalVolume = new G4LogicalVolume(
        pPTFEcathode, Teflon, "CathodeRingTopFrameLogicalVolume", 0, 0, 0);
        
  stringstream cathodeframename;
  for (int i = 0; i < dNbofPTFEpillar; i++) {
      G4RotationMatrix *zRot = new G4RotationMatrix();
      zRot->rotateZ(dPTFEpillarAngularStep * i);

      cathodeframename.str("");
      cathodeframename << "Teflon_CathodeRingFrame_" << i;
      m_pCathodeRingTopFramePhysicalVolume.push_back(
        new G4PVPlacement(zRot,
                          G4ThreeVector(0., 0., dPTFECathodeOffsetZ),
                          m_pCathodeRingTopFrameLogicalVolume,
                          cathodeframename.str(), m_pLXeLogicalVolume,
                          false, 0));

      cathodeframename.str("");
      cathodeframename << "Teflon_CathodeRingFrameLogicalBorderSurface_" << i;                          
      G4VPhysicalVolume *CathodeRingTopFramePhysicalVolume =
        (G4VPhysicalVolume *) m_pCathodeRingTopFramePhysicalVolume.at(i);
      new G4LogicalBorderSurface(cathodeframename.str(), 
                                 m_pLXePhysicalVolume,
                                 CathodeRingTopFramePhysicalVolume,
                                 Materials->LXeTeflonOpticalSurface());
  }

  //__________ Bottom mesh ring __________

  G4double dBMringHeight = GetGeometryParameterNT("BMringTotalHeight");
  G4double dBMringWidth = GetGeometryParameterNT("BMringTubeWidth");
  G4double dBMringTorusR = GetGeometryParameterNT("BMringTorusRadius");
  G4double dBMringTubeHeight = GetGeometryParameterNT("BMringTubeHeight");
  G4double dBMringCurvature =
            0.5 * GetGeometryParameterNT("BMringInnerDiameter");
  G4double dBMringOffsetZ = dCathodeRingOffsetZ - 0.5 * dCathodeRingTubeHeight
            - (dCathodeRingHeight - dCathodeRingTubeHeight)
            - dPTFECorrZ * GetGeometryParameterNT("BMringTopToCathodeRingBot")
            - (dBMringHeight - dBMringTubeHeight)
            - 0.5 * dBMringTubeHeight;

  G4Tubs *pBMringTube =
            new G4Tubs("BMringTube", dBMringCurvature,
                       dBMringCurvature + dBMringWidth,
                       0.5 * dBMringTubeHeight, 0, 2 * M_PI);
  G4Tubs *pBMringTubeTop =
    new G4Tubs("BMringTube", dBMringCurvature + dBMringTorusR,
               dBMringCurvature + dBMringWidth - dBMringTorusR,
               0.5 * (dBMringHeight - dBMringTubeHeight), 0, 2 * M_PI);
  G4Torus *pBMringTorus1 =
      new G4Torus("BMringTorus1", 0., dBMringTorusR,
                  dBMringCurvature + dBMringTorusR, 0, 2 * M_PI);
  G4Torus *pBMringTorus2 =
      new G4Torus("BMringTorus2", 0., dBMringTorusR,
                  dBMringCurvature + dBMringWidth - dBMringTorusR,
                  0, 2 * M_PI);

  G4UnionSolid *pBMring =
      new G4UnionSolid("BMring", pBMringTube, pBMringTubeTop, 0,
                       G4ThreeVector(0., 0., dBMringTubeHeight));
  pBMring =
      new G4UnionSolid("BMring", pBMring, pBMringTorus1, 0,
                       G4ThreeVector(0., 0., 0.5 * dBMringTubeHeight));
  pBMring =
      new G4UnionSolid("BMring", pBMring, pBMringTorus2, 0,
                       G4ThreeVector(0., 0., 0.5 * dBMringTubeHeight));
  m_pBottomMeshRingLogicalVolume = new G4LogicalVolume(
      pBMring, SS316Ti, "CathodeRingLogicalVolume", 0, 0, 0);
  m_pBottomMeshRingPhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(0., 0., dBMringOffsetZ),
      m_pBottomMeshRingLogicalVolume, "SS_BottomMeshRing",
      m_pLXeLogicalVolume, false, 0);

  //__________ PTFE ring below BM ring __________

  G4double dTeflonBMringHeight =
           dPTFECorrZ * GetGeometryParameterNT("BMringBotToPTFEReflectorTop");
  G4double dTeflonBMringInnerR =
           0.5 * GetGeometryParameterNT("TeflonBMringInnerD");
  G4double dTeflonBMringWidth = GetGeometryParameterNT("TeflonBMringWidth");
  G4double dTeflonBMringOffsetZ =
           dBMringOffsetZ - 0.5 * dBMringTubeHeight - 0.5 * dTeflonBMringHeight;

  G4Tubs *pPTFEring = new G4Tubs("PTFE_BMringTube", dTeflonBMringInnerR,
                                 dTeflonBMringInnerR + dTeflonBMringWidth,
                                 dTeflonBMringHeight * 0.5, 0, 2 * M_PI);

  m_pTeflonRingBelowBMringLogicalVolume = new G4LogicalVolume(
      pPTFEring, Teflon, "RingBelowBMringLogicalVolume", 0, 0, 0);
  m_pTeflonRingBelowBMringPhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(0., 0., dTeflonBMringOffsetZ),
      m_pTeflonRingBelowBMringLogicalVolume, "Teflon_RingBelowBottomMesh",
      m_pLXeLogicalVolume, false, 0);
      
  new G4LogicalBorderSurface("RingBelowBMringLogicalBorderSurface",
                             m_pLXePhysicalVolume, 
                             m_pTeflonRingBelowBMringPhysicalVolume,
                             Materials->LXeTeflonOpticalSurface());

  //__________ Bottom PTFE Reflector (identical to top one) __________

  G4double dBotReflectorRadius =
               GetGeometryParameterNT("BotReflectorDiameter") * 0.5;
  G4double dBotReflectorHeight = GetGeometryParameterNT("TopReflectorHeight");
  G4double dBotReflectorOffsetZ =
   dBMringOffsetZ - 0.5 * dBMringTubeHeight
   - dPTFECorrZ * GetGeometryParameterNT("BMringBotToPTFEReflectorTop")
   - 0.5 * dBotReflectorHeight;

  G4Tubs *pBotReflectorTube =
      new G4Tubs("BotReflectorTube", 0., dBotReflectorRadius,
                 dBotReflectorHeight * 0.5, 0, 2 * M_PI);

  // Here the four holes are merged, so there is only one subtraction later

  // The one going through the whole plate goes first (2nd one from the cone)
  G4double dBotReflectorHoleRadius =
        dPTFECorrR * GetGeometryParameterNT("TopReflectorConeHoleDmin") * 0.5;
  G4Tubs *pBotReflectorHoleBase =
      new G4Tubs("BotReflectorHole", 0., dBotReflectorHoleRadius,
                 dBotReflectorHeight * 0.5, 0, 2 * M_PI);
  
  // Merging conical hole  
  G4double dHoleConeRmax =
        dPTFECorrR * GetGeometryParameterNT("TopReflectorConeHoleDmax") * 0.5;
  G4double dHoleConeHeight=
               GetGeometryParameterNT("TopReflectorConeHoleHeight");
  G4Cons *pCutHoleCone =
     new G4Cons("CutHoleCone", 0., dHoleConeRmax, 0., dBotReflectorHoleRadius,
                dHoleConeHeight * 0.5, 0., 2 * M_PI);

  G4double zPosOverlap = 0.5 * (dHoleConeHeight - dBotReflectorHeight);
  G4UnionSolid *pBotReflectorCut =
      new G4UnionSolid("BotReflectorHole_2", pBotReflectorHoleBase, pCutHoleCone,
                       0, G4ThreeVector(0., 0., zPosOverlap));

  // Merging 3rd hold from cone (closest to window)
  G4double dHole3Radius =
        dPTFECorrR * GetGeometryParameterNT("TopReflectorTube2Diameter") * 0.5;
  G4double dHole3Height=
               GetGeometryParameterNT("TopReflectorTube2Height");
  G4Tubs *pCutHoleTube3 =
      new G4Tubs("CutHoleTube3", 0., dHole3Radius,
                 dHole3Height * 0.5, 0, 2 * M_PI);

  G4double dHole2Height = GetGeometryParameterNT("TopReflectorTube1Height");
  zPosOverlap = zPosOverlap + 0.5 * (dHoleConeHeight + dHole3Height)
                + dHole2Height;
  pBotReflectorCut =
      new G4UnionSolid("BotReflectorHole_3", pBotReflectorCut, pCutHoleTube3,
                       0, G4ThreeVector(0., 0., zPosOverlap));

  // Merging 4rd hold from cone
  G4double dHole4Radius =
        dPTFECorrR * GetGeometryParameterNT("TopReflectorTube3Diameter") * 0.5;
  G4double dHole4Height=
               GetGeometryParameterNT("TopReflectorTube3Height");
  G4Tubs *pCutHoleTube4 =
      new G4Tubs("CutHoleTube4", 0., dHole4Radius,
                 dHole4Height * 0.5, 0, 2 * M_PI);

  zPosOverlap = 0.5 * (dBotReflectorHeight - dHole4Height);
  pBotReflectorCut =
      new G4UnionSolid("BotReflectorCut", pBotReflectorCut, pCutHoleTube4,
                       0, G4ThreeVector(0., 0., zPosOverlap));

  //__________ Bottom copper plate __________

  G4double dBotCopperPlateRadius =
                GetGeometryParameterNT("BotCopperPlateDiameter") * 0.5;
  G4double dBotCopperPlateHeight = GetGeometryParameterNT("BotCopperPlateHeight");
  G4double dBotCopperPlateOffsetZ =
              dBotReflectorOffsetZ - 0.5 * dBotReflectorHeight
              - GetGeometryParameterNT("BotPTFEReflectorToBotCopperPlateTop")
              - 0.5 * dBotCopperPlateHeight;

  G4Tubs *pBotCopperPlateTube =
      new G4Tubs("BotCopperPlateTube", 0., dBotCopperPlateRadius,
                 dBotCopperPlateHeight * 0.5, 0, 2 * M_PI);

  // Hole for subtraction during PMTs placement
  G4double dBotCopperPlateHoleRadius =
                 GetGeometryParameterNT("BotCopperPlateHoleDiameter") * 0.5;

  G4Tubs *pBotCopperPlateCut =
      new G4Tubs("BotCopperPlateCut", 0., dBotCopperPlateHoleRadius,
                 dBotCopperPlateHeight * 0.5, 0, 2 * M_PI);

  //__________ Bottom PMTs holder (identical to top one) __________

  G4double dBotPMTholderRadius =
            GetGeometryParameterNT("TopPTFEholderDiameter") * 0.5;
  G4double dBotPMTholderHeight =
            GetGeometryParameterNT("TopPTFEholderHeight");
  G4double dBotPmtHolderOffsetZ =
            dBotCopperPlateOffsetZ - 0.5 * dBotCopperPlateHeight
            - GetGeometryParameterNT("BotCopperPlateToTopPTFEholder")
            - 0.5 * dBotPMTholderHeight;

  G4Tubs *pBotPmtHolderTube =
      new G4Tubs("BotPmtHolderTube", 0., dBotPMTholderRadius,
                 dBotPMTholderHeight * 0.5, 0, 2 * M_PI);

  // Hole for subtraction during PMTs placement
  G4double dBotPMTholderHoleRadius =
        dPTFECorrR * GetGeometryParameterNT("TopPTFEholderHoleDiameter") * 0.5;

  G4Tubs *pBotPmtHolderCut =
      new G4Tubs("BotPmtHolderCut", 0., dBotPMTholderHoleRadius,
                 dBotPMTholderHeight * 0.5, 0, 2 * M_PI);

  //__________ Bottom PMTs (R11410) __________

  G4int TotNbOfTopPMTs = G4int(GetGeometryParameterNT("NbOfTopPMTs"));
  G4int TotNbOfPMTs = G4int(GetGeometryParameterNT("NbOfPMTs"));
  G4double dPMTsOffsetZ = 
            dBotReflectorOffsetZ + 0.5 * dBotReflectorHeight
            - GetGeometryParameterNT("BotReflectorTopToPMTtop")
            - 0.5 * GetGeometryParameterNT("PMTheight");            

  //__________ Bases Top PMTs __________

  G4double dPmtBasesRadius = GetGeometryParameterNT("PmtBasesDiameter") * 0.5;
  G4double dPmtBasesHeight = GetGeometryParameterNT("PmtBasesHeight");
  G4double dBotPmtBasesOffsetZ =
            dPMTsOffsetZ - 0.5 * GetGeometryParameterNT("PMTheight")
            - GetGeometryParameterNT("TopPmtStemToBases")
            - 0.5 * dPmtBasesHeight;

  G4Tubs *pPmtBases = new G4Tubs("pPmtBases", 0., dPmtBasesRadius,
                                 dPmtBasesHeight * 0.5, 0, 2 * M_PI);
  m_pPmtBasesLogicalVolume =
      new G4LogicalVolume(pPmtBases, Cirlex, "PmtBasesLogicalVolume", 0, 0, 0);

  //__________ Placement of Bottom PMT array and Cuts for Plates __________

  // Placeholders for the holes subtraction
  G4SubtractionSolid *pBotPmtHolder;
  G4SubtractionSolid *pBotCopperPlate;
  G4SubtractionSolid *pBotReflector;

  stringstream hVolumeName;
  stringstream hVolumeName_bases;
  stringstream hHoleName_reflector;
  stringstream hHoleName_copper;
  stringstream hHoleName_ptfeholder;

  for (G4int iPMTNt = TotNbOfTopPMTs; iPMTNt < TotNbOfPMTs; ++iPMTNt) {
    G4ThreeVector PmtPosition;
    PmtPosition = GetPMTsPositionBottomArray(iPMTNt);
  
    // Placing PMTs and bases
    hVolumeName.str("");
    hVolumeName << "PmtTpcBot_" << iPMTNt;
    m_pPMTPhysicalVolumes.push_back(
       new G4PVPlacement(pRotX180,
                         PmtPosition + G4ThreeVector(0., 0., dPMTsOffsetZ),
                         m_pPmtR11410LogicalVolume, hVolumeName.str(),
                         m_pLXeLogicalVolume, false, iPMTNt));

    hVolumeName_bases.str("");
    hVolumeName_bases << "PmtBaseTpcBot_" << iPMTNt;
    m_pPmtBasesPhysicalVolumes.push_back(new G4PVPlacement(
        0, PmtPosition + G4ThreeVector(0., 0., dBotPmtBasesOffsetZ),
        m_pPmtBasesLogicalVolume, hVolumeName_bases.str(), m_pLXeLogicalVolume,
        false, iPMTNt));

    // Time to subtract holes at the PMT positions

    if (iPMTNt==TotNbOfTopPMTs) {
      hHoleName_ptfeholder.str("");
      hHoleName_ptfeholder << "BotPTFEholderWithCuts_" << iPMTNt;
      pBotPmtHolder =
           new G4SubtractionSolid(hHoleName_ptfeholder.str(), pBotPmtHolderTube,
                                  pBotPmtHolderCut, 0, PmtPosition);

      hHoleName_copper.str("");
      hHoleName_copper << "BotCopperWithCuts_" << iPMTNt;
      pBotCopperPlate =
            new G4SubtractionSolid(hHoleName_copper.str(), pBotCopperPlateTube,
                                   pBotCopperPlateCut, 0, PmtPosition);

      hHoleName_reflector.str("");
      hHoleName_reflector << "BotReflectorWithCuts_" << iPMTNt;
      pBotReflector =
            new G4SubtractionSolid(hHoleName_reflector.str(), pBotReflectorTube,
                                   pBotReflectorCut, 0, PmtPosition);
    }    
    else {
      hHoleName_ptfeholder.str("");
      hHoleName_ptfeholder << "BotPTFEholderWithCuts_" << iPMTNt;
      pBotPmtHolder =
           new G4SubtractionSolid(hHoleName_ptfeholder.str(), pBotPmtHolder,
                                  pBotPmtHolderCut, 0, PmtPosition);

      hHoleName_copper.str("");
      hHoleName_copper << "BotCopperWithCuts_" << iPMTNt;
      pBotCopperPlate =
            new G4SubtractionSolid(hHoleName_copper.str(), pBotCopperPlate,
                                   pBotCopperPlateCut, 0, PmtPosition);

      hHoleName_reflector.str("");
      hHoleName_reflector << "BotReflectorWithCuts_" << iPMTNt;
      pBotReflector =
            new G4SubtractionSolid(hHoleName_reflector.str(), pBotReflector,
                                   pBotReflectorCut, 0, PmtPosition);
    }        
  }

  m_pBottomPMTHolderLogicalVolume =
     new G4LogicalVolume(pBotPmtHolder, Teflon, "BottomPmtHolderLogicalVolume",
                         0, 0, 0);
  m_pBottomPMTHolderPhysicalVolume =
      new G4PVPlacement(0, G4ThreeVector(0., 0., dBotPmtHolderOffsetZ),
                        m_pBottomPMTHolderLogicalVolume, "Teflon_BottomPmtHolder",
                        m_pLXeLogicalVolume, false, 0);
                        
  new G4LogicalBorderSurface("BottomPmtHolderLogicalBorderSurface",
                             m_pLXePhysicalVolume, 
                             m_pBottomPMTHolderPhysicalVolume,
                             Materials->LXeTeflonOpticalSurface());

  m_pBottomPMTCopperLogicalVolume =
     new G4LogicalVolume(pBotCopperPlate, Copper,
                         "BottomCopperPlateLogicalVolume", 0, 0, 0);
  m_pBottomPMTCopperPhysicalVolume =
      new G4PVPlacement(0, G4ThreeVector(0., 0., dBotCopperPlateOffsetZ),
                        m_pBottomPMTCopperLogicalVolume, "Copper_BottomPmtPlate",
                        m_pLXeLogicalVolume, false, 0);

  m_pBottomPMTReflectorLogicalVolume =
      new G4LogicalVolume(pBotReflector, Teflon,
                          "BottomReflectorLogicalVolume", 0, 0, 0);
  m_pBottomPMTReflectorPhysicalVolume =
      new G4PVPlacement(pRotX180, G4ThreeVector(0., 0., dBotReflectorOffsetZ),
                        m_pBottomPMTReflectorLogicalVolume,
                        "Teflon_BottomReflector",
                        m_pLXeLogicalVolume, false, 0);
                        
  new G4LogicalBorderSurface("BottomReflectorLogicalBorderSurface",
                             m_pLXePhysicalVolume, 
                             m_pBottomPMTReflectorPhysicalVolume,
                             Materials->LXeTeflonOpticalSurface());

  //__________ Copper ring below pillars __________

  G4double dCuBelowPillarsInnerR =
       0.5 * GetGeometryParameterNT("CuBelowPillarsInnerDiameter");
  G4double dCuBelowPillarsHeight =
       GetGeometryParameterNT("CuBelowPillarsHeight");
  G4double dCuBelowPillarsWidth =
       GetGeometryParameterNT("CuBelowPillarsWidth");
  G4double dCuBellowPillarsOffsetZ =
       dBotCopperPlateOffsetZ + 0.5 * dBotCopperPlateHeight
       + 0.5 * dCuBelowPillarsHeight;

  G4Tubs *pCuRingBot =
      new G4Tubs("CopperRingBottom", dCuBelowPillarsInnerR,
                 dCuBelowPillarsInnerR + dCuBelowPillarsWidth,
                 0.5 * dCuBelowPillarsHeight, 0, 2 * M_PI);
  m_pLowerRingLogicalVolume = new G4LogicalVolume(
      pCuRingBot, Copper, "LowerRingLogicalVolume", 0, 0, 0);
  m_pLowerRingPhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(0., 0., dCuBellowPillarsOffsetZ), m_pLowerRingLogicalVolume,
      "Copper_LowerRing", m_pLXeLogicalVolume, false, 0);

  //==== attributes ====
  G4Colour hCopperColor(1., 0.757, 0.24, 0.1);
  G4VisAttributes *pCopperVisAtt = new G4VisAttributes(hCopperColor);
  pCopperVisAtt->SetVisibility(true);
  m_pLowerRingLogicalVolume->SetVisAttributes(pCopperVisAtt);
  m_pFieldShaperRingLogicalVolume->SetVisAttributes(pCopperVisAtt);
  m_pFieldGuardLogicalVolume->SetVisAttributes(pCopperVisAtt);
  
  G4Colour hPMTCopperColor(1., 0.757, 0.24, 0.1);
  G4VisAttributes *pPMTCopperVisAtt = new G4VisAttributes(hPMTCopperColor);
  pPMTCopperVisAtt->SetVisibility(false);
  m_pBottomPMTCopperLogicalVolume->SetVisAttributes(pPMTCopperVisAtt);

  G4Colour hTeflonColor(0.5, 0.3, 0.2, 0.01);
  G4VisAttributes *pTeflonVisAtt = new G4VisAttributes(hTeflonColor);
  pTeflonVisAtt->SetVisibility(true);
  m_pTpcLogicalVolume->SetVisAttributes(pTeflonVisAtt);
  m_pBottomTpcLogicalVolume->SetVisAttributes(pTeflonVisAtt);
  m_pCathodeRingTopFrameLogicalVolume->SetVisAttributes(pTeflonVisAtt);
  m_pTeflonRingBelowBMringLogicalVolume->SetVisAttributes(pTeflonVisAtt);
  pTeflonVisAtt->SetVisibility(false);
  m_pPTFEpillarLogicalVolume->SetVisAttributes(pTeflonVisAtt);

  G4Colour hPMTTeflonColor(0.5, 0.3, 0.2, 0.01);
  G4VisAttributes *pPMTTeflonVisAtt = new G4VisAttributes(hPMTTeflonColor);
  pPMTTeflonVisAtt->SetVisibility(false);
  m_pBottomPMTReflectorLogicalVolume->SetVisAttributes(pPMTTeflonVisAtt);
  m_pBottomPMTHolderLogicalVolume->SetVisAttributes(pPMTTeflonVisAtt);
  
  G4Colour hSSColor(0.600, 0.600, 0.600, 0.1);
  G4VisAttributes *pSSVisAtt = new G4VisAttributes(hSSColor);
  pSSVisAtt->SetVisibility(true);
  m_pCathodeMeshRingLogicalVolume->SetVisAttributes(pSSVisAtt);
  m_pBottomMeshRingLogicalVolume->SetVisAttributes(pSSVisAtt);
}

void XenonNtTPC::ConstructGrids() {
  // G4Material *GridMeshAluminium =
  //     G4Material::GetMaterial("GridMeshAluminium");
  G4Material *AnodeMesh = G4Material::GetMaterial("AnodeMesh");
  G4Material *TopScreeningMesh = G4Material::GetMaterial("TopScreeningMesh");
  G4Material *GateMesh = G4Material::GetMaterial("GateMesh");
  G4Material *CathodeMesh = G4Material::GetMaterial("CathodeMesh");
  G4Material *BottomScreeningMesh = 
      G4Material::GetMaterial("BottomScreeningMesh");

  //G4double dTorlonCorrZ = 1 - GetGeometryParameterNT("Torlon_ShrinkageZ");
  G4double dPTFECorrZ = 1 - GetGeometryParameterNT("PTFE_ShrinkageZ");

  // DR 20181029 - Copying the z positioning from 'ConstructTopTPC' and
  //               'ConstructMainTPC', such that everything is propagated
  //               in exactly the same way.

  //~~~~~~~
    // Bell
    G4double dBellPlateHeight = GetGeometryParameterNT("BellPlateHeight");
    G4double dBellPlateOffsetZ = GetGeometryParameterNT("BellPlateOffsetZ");
    G4double dBellWallHeight = GetGeometryParameterNT("BellWallHeight");
    G4double dBellWallOffsetZ = dBellPlateOffsetZ + 0.5 * dBellPlateHeight
                                - 0.5 * dBellWallHeight;
    // Copper ring below bell
    G4double dCuRingHeight = GetGeometryParameterNT("CopperRingHeight");  
    G4double dCuRingOffsetZ = dBellWallOffsetZ - 0.5 * dBellWallHeight
                              - 0.5 * dCuRingHeight;
    // Gate ring
    G4double dGateRingTotalHeight =
           GetGeometryParameterNT("GateRingTotalHeight");
    G4double dGateRingOffsetZ = dCuRingOffsetZ + 0.5 * dCuRingHeight
           + dPTFECorrZ * GetGeometryParameterNT("BellWallBotToGateRingBot")
           + 0.5 * dGateRingTotalHeight;
    // Anode ring
    G4double dAnodeRingHeight = GetGeometryParameterNT("AnodeRingHeight");
    G4double dAnodeRingOffsetZ = dGateRingOffsetZ + 0.5 * dGateRingTotalHeight
        + dPTFECorrZ * GetGeometryParameterNT("GateRingTopToAnodeRingBot")
        + 0.5 * dAnodeRingHeight;
    // Top Mesh ring
    G4double dTopMeshRingHeight = GetGeometryParameterNT("TopMeshRingHeight");
    G4double dTopMeshRingOffsetZ = dAnodeRingOffsetZ + 0.5 * dAnodeRingHeight
        + dPTFECorrZ * GetGeometryParameterNT("AnodeRingTopToTopMeshRingBot")
        + 0.5 * dTopMeshRingHeight;
  //~~~~~~~

  //~~~~~~~
    // TPC wall
    G4double dTpcHeight = dPTFECorrZ * GetGeometryParameterNT("TpcWallHeight");
    G4double dTpcOffsetZ = dGateRingOffsetZ + 0.5 * dGateRingTotalHeight
           - GetGeometryParameterNT("TopGateRingToTopTPC")
           - 0.5 * dTpcHeight;
    // Cathode ring
    G4double dCathodeRingHeight = GetGeometryParameterNT("CathodeRingTotalHeight");
    G4double dCathodeRingTubeHeight =
           GetGeometryParameterNT("CathodeRingTubeHeight");
    G4double dCathodeRingOffsetZ = dTpcOffsetZ - 0.5 * dTpcHeight
           - dPTFECorrZ * GetGeometryParameterNT("TpcBotToCathodeRingTop")
           - 0.5 * dCathodeRingTubeHeight;
    // Bottom mesh ring
    G4double dBMringHeight = GetGeometryParameterNT("BMringTotalHeight");
    G4double dBMringTubeHeight = GetGeometryParameterNT("BMringTubeHeight");
    G4double dBMringOffsetZ = dCathodeRingOffsetZ - 0.5 * dCathodeRingTubeHeight
           - (dCathodeRingHeight - dCathodeRingTubeHeight)
           - dPTFECorrZ * GetGeometryParameterNT("BMringTopToCathodeRingBot")
           - (dBMringHeight - dBMringTubeHeight)
           - 0.5 * dBMringTubeHeight;
  //~~~~~~~

  //__________ Top Mesh __________

  G4double dTopMeshThickness = GetGeometryParameterNT("TopMeshThickness");
  G4double dTopMeshRadius = 0.5 * GetGeometryParameterNT("TopMeshDiameter");
  G4double dTopMeshOffsetZ = dTopMeshRingOffsetZ - 0.5 * dTopMeshRingHeight
                             + 0.5 * dTopMeshThickness;

  G4Tubs *pTopMesh =
      new G4Tubs("TopMesh", 0., dTopMeshRadius, 0.5 * dTopMeshThickness,
                 0, 2 * M_PI);
  m_pTopGridMeshLogicalVolume = new G4LogicalVolume(
       pTopMesh, TopScreeningMesh, "TopMeshLogicalVolume", 0, 0, 0);
  m_pTopGridMeshPhysicalVolume = new G4PVPlacement(
       0, G4ThreeVector(0., 0., dTopMeshOffsetZ), m_pTopGridMeshLogicalVolume,
       "GridMeshAluminium_TopMesh", m_pGXeLogicalVolume, false, 0);

  //__________ Anode Mesh __________

  G4double dAnodeThickness = GetGeometryParameterNT("AnodeMeshThickness");
  G4double dAnodeRadius = 0.5 * GetGeometryParameterNT("AnodeMeshDiameter");
  G4double dAnodeOffsetZ = dAnodeRingOffsetZ - 0.5 * dAnodeRingHeight
                           + 0.5 * dAnodeThickness;

  G4Tubs *pAnodeMesh =
      new G4Tubs("Anode", 0., dAnodeRadius, 0.5 * dAnodeThickness,
                 0, 2 * M_PI);
  m_pAnodeMeshLogicalVolume = new G4LogicalVolume(
       pAnodeMesh, AnodeMesh, "AnodeLogicalVolume", 0, 0, 0);
  m_pAnodeMeshPhysicalVolume = new G4PVPlacement(
       0, G4ThreeVector(0., 0., dAnodeOffsetZ), m_pAnodeMeshLogicalVolume,
       "GridMeshAluminium_AnodeMesh", m_pGXeLogicalVolume, false, 0);

  //__________ Gate Mesh __________

  G4double dGateMeshThickness = GetGeometryParameterNT("GateMeshThickness");
  G4double dGateMeshRadius = 0.5 * GetGeometryParameterNT("GateMeshDiameter");
  G4double dGateMeshOffsetZ = dGateRingOffsetZ + 0.5 * dGateRingTotalHeight
                             - 0.5 * dGateMeshThickness;

  G4Tubs *pGroundMesh =
      new G4Tubs("Gate", 0., dGateMeshRadius, 0.5 * dGateMeshThickness,
                 0, 2 * M_PI);
  m_pGroundMeshLogicalVolume = new G4LogicalVolume(
       pGroundMesh, GateMesh, "GroundMeshLogicalVolume", 0, 0, 0);
  m_pGroundMeshPhysicalVolume = new G4PVPlacement(
       0, G4ThreeVector(0., 0., dGateMeshOffsetZ), m_pGroundMeshLogicalVolume,
       "GridMeshAluminium_GroundMesh", m_pLXeLogicalVolume, false, 0);

  //__________ Cathode Mesh __________

  G4double dCathodeThickness = GetGeometryParameterNT("CathodeMeshThickness");
  G4double dCathodeRadius =
         0.5 * GetGeometryParameterNT("CathodeMeshDiameter");
  G4double dCathodeOffsetZ = dCathodeRingOffsetZ + 0.5 * dCathodeRingTubeHeight
         - 0.5 * dCathodeThickness;

  G4Tubs *pCathodeMesh =
      new G4Tubs("Cathode", 0., dCathodeRadius, 0.5 * dCathodeThickness,
                 0, 2 * M_PI);
  m_pCathodeMeshLogicalVolume = new G4LogicalVolume(
       pCathodeMesh, CathodeMesh, "GroundMeshLogicalVolume", 0, 0, 0);
  m_pCathodeMeshPhysicalVolume = new G4PVPlacement(
       0, G4ThreeVector(0., 0., dCathodeOffsetZ), m_pCathodeMeshLogicalVolume,
       "GridMeshAluminium_CathodeMesh", m_pLXeLogicalVolume, false, 0);

  //__________ Bottom Mesh __________

  G4double dBottomMeshThickness = GetGeometryParameterNT("BottomMeshThickness");
  G4double dBottomMeshRadius =
         0.5 * GetGeometryParameterNT("BottomMeshDiameter");
  G4double dBottomMeshOffsetZ = dBMringOffsetZ - 0.5 * dBMringTubeHeight
         + 0.5 * dBottomMeshThickness;

  G4Tubs *pBottomMesh =
      new G4Tubs("BottomMesh", 0., dBottomMeshRadius, 0.5 * dBottomMeshThickness,
                 0, 2 * M_PI);
  m_pBottomGridMeshLogicalVolume = new G4LogicalVolume(
       pBottomMesh, BottomScreeningMesh, "GroundMeshLogicalVolume", 0, 0, 0);
  m_pBottomGridMeshPhysicalVolume = new G4PVPlacement(
       0, G4ThreeVector(0., 0., dBottomMeshOffsetZ),
      m_pBottomGridMeshLogicalVolume, "GridMeshAluminium_BottomMesh",
      m_pLXeLogicalVolume, false, 0);

  //==== attributes ====
  G4Colour hGridColor(0.4, 0.5, 0.7, 0.01);
  G4VisAttributes *pGridVisAtt = new G4VisAttributes(hGridColor);
  pGridVisAtt->SetVisibility(true);
  m_pTopGridMeshLogicalVolume->SetVisAttributes(pGridVisAtt);
  m_pAnodeMeshLogicalVolume->SetVisAttributes(pGridVisAtt);
  m_pGroundMeshLogicalVolume->SetVisAttributes(pGridVisAtt);
  m_pCathodeMeshLogicalVolume->SetVisAttributes(pGridVisAtt);
  m_pBottomGridMeshLogicalVolume->SetVisAttributes(pGridVisAtt);
}

//=============================== Pillars ====================================
G4UnionSolid *XenonNtTPC::ConstructPillar() {
  G4double dPTFECorrZ = 1 - GetGeometryParameterNT("PTFE_ShrinkageZ");

  G4double dBottomBoxbase_x = GetGeometryParameterNT("BottomBoxBase_x");
  G4double dBottomBoxbase_y = GetGeometryParameterNT("BottomBoxBase_y");
  G4double dBottomBox_height =
       dPTFECorrZ * GetGeometryParameterNT("BottomBox_height");

  G4double dTrapezoidbase_y1 = GetGeometryParameterNT("Trapezoid_y1");
  G4double dTrapezoidbase_y2 = GetGeometryParameterNT("Trapezoid_y2");
  G4double dTrapezoid_height =
       dPTFECorrZ * GetGeometryParameterNT("Trapezoid_height");

  G4double dMiddleBoxbase_y = GetGeometryParameterNT("Trapezoid_y1");
  G4double dMiddleBox_height =
       dPTFECorrZ * GetGeometryParameterNT("MiddleBox_height");
  G4double dMiddleBox_height_ToTrapezoid =
       dPTFECorrZ * GetGeometryParameterNT("MiddleBox_height_UpToTrapezoid");

  G4double dTopBoxbase_y = GetGeometryParameterNT("TopBoxBase_y");
  G4double dTopBox_height =
       dPTFECorrZ * GetGeometryParameterNT("TopBox_height");

  G4Box *p_MidBox = new G4Box("PillarMiddleBox", dBottomBoxbase_x * 0.5,
                              dMiddleBoxbase_y * 0.5, dMiddleBox_height * 0.5);
  G4Box *p_TopBox = new G4Box("PillarTopBox", dBottomBoxbase_x * 0.5,
                              dTopBoxbase_y * 0.5, dTopBox_height * 0.5);
  G4Trd *p_Trapezoid =
      new G4Trd("PillarTrapezoid", dBottomBoxbase_x * 0.5, dBottomBoxbase_x * 0.5,
                (dTrapezoidbase_y2 - dTrapezoidbase_y1), 0.,
                dTrapezoid_height * 0.5);
  G4Box *p_BotBox =
      new G4Box("PillarBottomBox", dBottomBoxbase_x * 0.5, dBottomBoxbase_y * 0.5,
                dBottomBox_height * 0.5);

  G4double yPos, zPos;
  G4UnionSolid *pPTFEPillar;

  zPos = dMiddleBox_height * 0.5 + dTopBox_height * 0.5;
  yPos = (dTopBoxbase_y - dMiddleBoxbase_y) * 0.5;
  pPTFEPillar = new G4UnionSolid("PTFEpillar", p_MidBox, p_TopBox, 0,
                                 G4ThreeVector(0., yPos, zPos));

  zPos = dMiddleBox_height * 0.5 - dMiddleBox_height_ToTrapezoid
         - dTrapezoid_height * 0.5;
  yPos = dMiddleBoxbase_y * 0.5;
  pPTFEPillar = new G4UnionSolid("UnionSolid", pPTFEPillar, p_Trapezoid, 0,
                               G4ThreeVector(0., yPos, zPos));

  zPos = zPos - (dTrapezoid_height + dBottomBox_height) * 0.5;
  yPos = yPos + dTrapezoidbase_y2 - dTrapezoidbase_y1
         - dBottomBoxbase_y * 0.5;
  pPTFEPillar = new G4UnionSolid("UnionSolid", pPTFEPillar, p_BotBox, 0,
                                 G4ThreeVector(0., yPos, zPos));

  return pPTFEPillar;
}

//============================ Frame around rings ============================
G4SubtractionSolid *XenonNtTPC::ConstructTopRingsFrame() {
  G4double dPTFECorrZ = 1 - GetGeometryParameterNT("PTFE_ShrinkageZ");

  // Parameters copied from grids and rings construction
  //~~~~~~~
    G4double dGateRingTotalHeight =
             GetGeometryParameterNT("GateRingTotalHeight");
    G4double dGateRingTotalWidth = GetGeometryParameterNT("GateRingTotalWidth");  
    G4double dGateRingTopInnerRadius =
          0.5 * GetGeometryParameterNT("GateRingInnerDiameterMin");
    G4double dAnodeRingHeight = GetGeometryParameterNT("AnodeRingHeight");
    G4double dAnodeRingWidth = GetGeometryParameterNT("AnodeRingWidth");
    G4double dAnodeRingInnerR =
          0.5 * GetGeometryParameterNT("AnodeRingInnerDiameter");
    G4double dTopMeshRingHeight = GetGeometryParameterNT("TopMeshRingHeight");
    G4double dTopMeshRingInnerR =
          0.5 * GetGeometryParameterNT("TopMeshRingInnerDiameter");
    G4double dTopMeshRingWidth = GetGeometryParameterNT("TopMeshRingWidth");
  //~~~~~~~

  G4double dHeightThinFrame =
      dPTFECorrZ * GetGeometryParameterNT("ThinElectrodesFrameHeight");
  G4double dHeightFrame =
      dPTFECorrZ * GetGeometryParameterNT("ElectrodesFrameHeight")
      + dHeightThinFrame;
  G4double dInnerRadiusFrame =
      0.5 * GetGeometryParameterNT("TpcWallDiameter");
  G4double dMaxWidthFrame = GetGeometryParameterNT("ElectrodesFrameWidth");
  G4double dMinWidthFrame = GetGeometryParameterNT("ThinElectrodesFrameWidth");

  G4Tubs *pTopFrameTube = new G4Tubs("TopFrameTube", dInnerRadiusFrame,
                                        dInnerRadiusFrame + dMaxWidthFrame,
                                        0.5 * dHeightFrame, 0, 2 * M_PI);
  G4Tubs *pTopFrameCut = new G4Tubs("TopFrameCut",
                                       dInnerRadiusFrame + dMinWidthFrame,
                                       dInnerRadiusFrame + dMaxWidthFrame,
                                       0.5 * dHeightThinFrame, 0, 2 * M_PI);

  G4SubtractionSolid *pTopElectrodeFrame = new G4SubtractionSolid(
      "pTopElectrodeFrameGXe", pTopFrameTube, pTopFrameCut, 0,
      G4ThreeVector(0., 0., 0.5 * (dHeightFrame - dHeightThinFrame)));

  //Subtract enlarged gate ring tube (smaller inner diameter)
  G4double zPosSub =
      - 0.5 * dHeightFrame
      + 0.5 * dPTFECorrZ * dGateRingTotalHeight;
      // Because the ring moves down with the piece
  G4Tubs *pGateMeshRingCut =
      new G4Tubs("GateRingCut", 0,
                 dGateRingTopInnerRadius + dGateRingTotalWidth,
                 0.5 * dPTFECorrZ * dGateRingTotalHeight, 0, 2 * M_PI);

  pTopElectrodeFrame = new G4SubtractionSolid(
      "pTopElectrodeFrameGXe", pTopElectrodeFrame, pGateMeshRingCut, 0,
      G4ThreeVector(0., 0., zPosSub));

  //Subtract anode ring
  zPosSub =
      zPosSub
      + 0.5 * dPTFECorrZ * dGateRingTotalHeight
      + dPTFECorrZ * GetGeometryParameterNT("GateRingTopToAnodeRingBot")
      + 0.5 * dAnodeRingHeight;
  G4Tubs *AnodeRingCut =
      new G4Tubs("AnodeRingCut", dAnodeRingInnerR,
                 dAnodeRingInnerR + dAnodeRingWidth,
                 0.5 * dAnodeRingHeight, 0, 2 * M_PI);

  pTopElectrodeFrame = new G4SubtractionSolid(
      "pTopElectrodeFrameGXe", pTopElectrodeFrame, AnodeRingCut,
      0, G4ThreeVector(0., 0., zPosSub));

  //Subtract top mesh ring
  zPosSub =
      zPosSub
      + 0.5 * dAnodeRingHeight
      + dPTFECorrZ * GetGeometryParameterNT("AnodeRingTopToTopMeshRingBot")
      + 0.5 * dTopMeshRingHeight;
  G4Tubs *TopMeshRingCut =
      new G4Tubs("TopMeshRingCut", dTopMeshRingInnerR,
                 dTopMeshRingInnerR + dTopMeshRingWidth,
                 0.5 * dTopMeshRingHeight, 0, 2 * M_PI);

  pTopElectrodeFrame = new G4SubtractionSolid(
      "pTopElectrodeFrameGXe", pTopElectrodeFrame, TopMeshRingCut,
      0, G4ThreeVector(0., 0., zPosSub));
    
  //Subtract inlet above gate ring
  G4double dSmallInletHeight =
      dPTFECorrZ * GetGeometryParameterNT("FrameInletAboveGateRingHeight");
  G4double dBetweenRingsOuterR =
      GetGeometryParameterNT("ElectrodesFrameAboveGateRingsInnerR");
  zPosSub =
      - 0.5 * dHeightFrame
      + dPTFECorrZ * dGateRingTotalHeight
      + 0.5 * dSmallInletHeight;
  G4Tubs *pInletAboveGateRingCut =
      new G4Tubs("SmallInlet", 0., dBetweenRingsOuterR,
                 0.5 * dSmallInletHeight, 0, 2 * M_PI);

  pTopElectrodeFrame = new G4SubtractionSolid(
      "pTopElectrodeFrameGXe", pTopElectrodeFrame,
      pInletAboveGateRingCut, 0, G4ThreeVector(0., 0., zPosSub));

  //Subtract inlet above gate ring frame
  dBetweenRingsOuterR =
      GetGeometryParameterNT("ElectrodesFrameBetweenRingsInnerR");
  G4double dInletHeight =
      dPTFECorrZ * GetGeometryParameterNT("FrameAnodeToGateHeight");
  zPosSub =
      zPosSub
      - 0.5 * dSmallInletHeight
      + dPTFECorrZ * (GetGeometryParameterNT("FrameGateHeight")
                        - dGateRingTotalHeight)
      + 0.5 * dInletHeight;
  G4Tubs *pInletAboveGateFrameCut =
      new G4Tubs("Inlet", 0, dBetweenRingsOuterR,
                 0.5 * dInletHeight, 0, 2 * M_PI);

  pTopElectrodeFrame = new G4SubtractionSolid(
      "pTopElectrodeFrameGXe", pTopElectrodeFrame,
      pInletAboveGateFrameCut, 0, G4ThreeVector(0., 0., zPosSub));

  //Subtract inlet above anode ring frame
  G4double dInletHeight2 =
      dPTFECorrZ * GetGeometryParameterNT("FrameTopMeshToAnodeHeight");
  zPosSub =
      zPosSub
      + 0.5 * dInletHeight
      + dPTFECorrZ * GetGeometryParameterNT("FrameAnodeHeight")
      + 0.5 * dInletHeight2;
  G4Tubs *pInletAboveAnodeFrameCut =
      new G4Tubs("Inlet2", 0, dBetweenRingsOuterR,
                 0.5 * dInletHeight2, 0, 2 * M_PI);

  pTopElectrodeFrame = new G4SubtractionSolid(
     "pTopElectrodeFrameGXe", pTopElectrodeFrame,
      pInletAboveAnodeFrameCut, 0, G4ThreeVector(0., 0., zPosSub));

  //Subtract gas hole (tube) above the rings
  G4double dHoleRadius =
      dPTFECorrZ * GetGeometryParameterNT("FrameGasFeedthroughRadius");
  G4double dHoleHeight = 5. * mm; // Just thicker than the frame at that height
  G4RotationMatrix *pRotX90 = new G4RotationMatrix();
  pRotX90->rotateX(90. * deg);
  zPosSub =
      0.5 * dHeightFrame
      - dPTFECorrZ * GetGeometryParameterNT("FrameGasFeedthroughToTopFrame");
  G4Tubs *pGasTubeCut = new G4Tubs("GasHole", 0, dHoleRadius,
                                   dHoleHeight, 0, 2 * M_PI);

  pTopElectrodeFrame = new G4SubtractionSolid(
     "pTopElectrodeFrameGXe", pTopElectrodeFrame,
      pGasTubeCut, pRotX90, G4ThreeVector(0., dInnerRadiusFrame, zPosSub));

  return pTopElectrodeFrame;
}

//============================== PMT arrays ==================================
G4ThreeVector XenonNtTPC::GetPMTsPositionTopArray_rad(G4int iPMTNb) {
  // By Diego Ramírez, based on Marc Schumann's design:
  // https://xe1t-wiki.lngs.infn.it/doku.php?id=xenon:xenonnt:dsg:tpc:newdraft

  G4double dPTFECorrR = 1 - GetGeometryParameterNT("PTFE_ShrinkageR");

  G4ThreeVector hPos;
  iPMTNb++;  // to make array start at zero instead of one

  G4int nbPmt[9] = {1, 6, 12, 18, 24, 32, 38, 44, 50};  // 225 in total

  G4double pmtRadius[9] = {0., 82., 163, 245, 327, 408, 489, 570, 651};
  for(int i = 0; i < 9; i++) pmtRadius[i] *= dPTFECorrR;

  G4double startAngle[9] = {0., 60., 30., 20., 15., 11.25,
                            9.47368, 8.18182, 7.2};

  G4int pmtRing = 0;
  G4int iTotal = nbPmt[0];
  G4int iPMTNb_reset = iTotal + 1;

  while (iPMTNb > iTotal) {
    iPMTNb_reset = iPMTNb - (iTotal + 1);
    pmtRing++;
    iTotal += nbPmt[pmtRing];
  }

  G4double radius = pmtRadius[pmtRing];
  G4double angle =
      (startAngle[pmtRing] + iPMTNb_reset * 360. / (double)nbPmt[pmtRing]) *
      M_PI / 180.;

  iPMTNb--;  // to make array start at zero instead of one

  if (iVerbosityLevel >= 1)
    G4cout << "iPMTNb_top " << iPMTNb << "  -->  x = " << radius * cos(angle)
           << ",  y = " << radius * sin(angle)
           << ",  iPMTNb_reset = " << iPMTNb_reset << G4endl;

  hPos.setZ(0. * cm);
  hPos.setX(radius * cos(angle));
  hPos.setY(radius * sin(angle));

  return hPos;
}

G4ThreeVector XenonNtTPC::GetPMTsPositionTopArray_hex(G4int iPMTNb) {
  // By Diego Ramírez, based on Marc Schumann's design:
  // https://xe1t-wiki.lngs.infn.it/doku.php?id=xenon:xenonnt:dsg:tpc:newdraft

  G4double dPTFECorrR = 1 - GetGeometryParameterNT("PTFE_ShrinkageR");

  G4int nbPmt[19] = {6,  9,  12, 13, 14, 15, 16, 17, 16, 17,
                     16, 17, 16, 15, 14, 13, 12, 9,  6};
  
  iPMTNb++;  // to make array start at zero instead of one

  G4double y_start = 631.3329 * dPTFECorrR;
  G4double y = y_start;
  G4double x;

  G4ThreeVector hPos;

  G4int pmtRing = 0;
  G4int iTotal = nbPmt[0];
  G4int iPMTNb_reset = iTotal + 1;

  if (iPMTNb <= iTotal) {
    iPMTNb_reset = iPMTNb;
    y = y_start - pmtRing * 70.1481 * dPTFECorrR;
  }

  while (iPMTNb > iTotal) {
    iPMTNb_reset = iPMTNb - (iTotal);
    pmtRing++;
    y = y_start - pmtRing * 70.1481 * dPTFECorrR;
    iTotal += nbPmt[pmtRing];
  }

  if (nbPmt[pmtRing] % 2)
    x = -1. * (G4int)(nbPmt[pmtRing] / 2.) * 81. * dPTFECorrR; 
  else
    x = -1. * (G4int)(nbPmt[pmtRing] / 2.) * 81. * dPTFECorrR
        + 40.5 * dPTFECorrR;

  x = x + (iPMTNb_reset - 1) * 81. * dPTFECorrR;

  iPMTNb--;  // to make array start at zero instead of one

  if (iVerbosityLevel >= 1)
    G4cout << "iPMTNb_top " << iPMTNb << "  -->  x = " << x << ",  y = " << y
           << ",  pmtRing = " << pmtRing << ",  iPMTNb_reset = " << iPMTNb_reset
           << G4endl;

  hPos.setZ(0. * cm);
  hPos.setX(x);
  hPos.setY(y);

  return hPos;
}

G4ThreeVector XenonNtTPC::GetPMTsPositionBottomArray(G4int iPMTNb) {
  // By Diego Ramírez, based on Marc Schumann's design:
  // https://xe1t-wiki.lngs.infn.it/doku.php?id=xenon:xenonnt:dsg:tpc:newdraft

  G4double dPTFECorrR = 1 - GetGeometryParameterNT("PTFE_ShrinkageR");

  G4int nbPmtb[19] = {4,  9,  10, 13, 14, 15, 16, 15, 16, 17,
                      16, 15, 16, 15, 14, 13, 10, 9,  4};

  iPMTNb++;  // to make array start at zero instead of one

  G4double y_start = 631.3329 * dPTFECorrR;
  G4double y = y_start;
  G4double x;
  G4int iNbOfTopPMTs = G4int(GetGeometryParameterNT("NbOfTopPMTs"));
  G4int iPMTNb_bottom = iPMTNb - iNbOfTopPMTs;

  G4ThreeVector hPos;

  G4int pmtRing = 0;
  G4int iTotal = nbPmtb[0];
  G4int iPMTNb_reset = iPMTNb_bottom;

  while (iPMTNb_bottom > iTotal) {
    iPMTNb_reset = iPMTNb_bottom - (iTotal);
    pmtRing++;
    y = y_start - pmtRing * 70.1481 * dPTFECorrR;
    iTotal += nbPmtb[pmtRing];
  }

  // Cold dimensions
  if (nbPmtb[pmtRing] % 2)
    x = -1. * (G4int)(nbPmtb[pmtRing] / 2.) * 81. * dPTFECorrR;
  else
    x = -1. * (G4int)(nbPmtb[pmtRing] / 2.) * 81. * dPTFECorrR
        + 40.5 * dPTFECorrR;

  x = x + (iPMTNb_reset - 1) * 81. * dPTFECorrR;
        
  iPMTNb--;  // to make array start at zero instead of one

  if (iVerbosityLevel >= 1)
    G4cout << "iPMTNb " << iPMTNb << "  -->  x = " << x << ",  y = " << y
           << ",  pmtRing = " << pmtRing << ",  iPMTNb_reset = " << iPMTNb_reset
           << G4endl;

  hPos.setZ(0. * cm);
  hPos.setX(x);
  hPos.setY(y);

  return hPos;
}

//========================= Geometry information =============================
void XenonNtTPC::PrintGeometryInformation() {
  G4cout << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ " << G4endl;
  const G4double dLXeMass = m_pLXeLogicalVolume->GetMass(false, false) / kg;
  G4cout << "\nLXe Mass:                       " << dLXeMass << " kg" << G4endl;
  const G4double dGXeMass = m_pGXeLogicalVolume->GetMass(false, false) / kg;
  G4cout << "GXe Mass:                       " << dGXeMass << " kg" << G4endl;
  const G4double dBellPlateMass =
                        m_pBellPlateLogicalVolume->GetMass(false, false) / kg;
  G4cout << "\nBell Plate Mass:                " << dBellPlateMass << " kg"
         << G4endl;
  const G4double dBellWallMass =
                        m_pBellWallLogicalVolume->GetMass(false, false) / kg;
  G4cout << "Bell Wall Mass:                 " << dBellWallMass << " kg"
         << G4endl;
  const G4double dCuRingMass =
                        m_pCuRingLogicalVolume->GetMass(false, false) / kg;
  G4cout << "Top Copper Ring Mass:           " << dCuRingMass << " kg"
         << G4endl;
  const G4double dTopPmtHolderMass =
                        m_pTopPMTHolderLogicalVolume->GetMass(false, false) / kg;
  G4cout << "Top PMT Holder (PTFE) Mass:     " << dTopPmtHolderMass << " kg"
         << G4endl;
  const G4double dTopCuPlateMass =
                        m_pTopPMTCopperLogicalVolume->GetMass(false, false) / kg;
  G4cout << "Top PMT Copper Plate Mass:      " << dTopCuPlateMass << " kg"
         << G4endl;
  const G4double dTopReflectorMass =
                        m_pTopPMTReflectorLogicalVolume->GetMass(false, false) / kg;
  G4cout << "Top PTFE Reflector Mass:        " << dTopReflectorMass << " kg"
         << G4endl;

  const G4double dGateRingMass =
                        m_pGateRingLogicalVolume->GetMass(false, false) / kg;
  G4cout << "Gate ring Mass:        " << dGateRingMass << " kg"
         << G4endl;

 const G4double dAnodeMass =
                        m_pAnodeRingLogicalVolume->GetMass(false, false) / kg;
  G4cout << "Anode ring Mass:        " << dAnodeMass << " kg"
         << G4endl;

 const G4double dCathodeMeshRingMass =
                        m_pCathodeMeshRingLogicalVolume->GetMass(false, false) / kg;
  G4cout << "Cathode mesh ring Mass:        " << dCathodeMeshRingMass << " kg"
         << G4endl;

 const G4double dTopMeshRingMass =
                        m_pTopMeshRingLogicalVolume->GetMass(false, false) / kg;
  G4cout << "Top mesh ring Mass:        " << dTopMeshRingMass << " kg"
         << G4endl;

 const G4double dBottomMeshRingMass =
                        m_pBottomMeshRingLogicalVolume->GetMass(false, false) / kg;
  G4cout << "bottom mesh ring Mass:        " << dBottomMeshRingMass << " kg"
         << G4endl;

  const G4double dLowerRingMass =
                        m_pLowerRingLogicalVolume->GetMass(false, false) / kg;
  G4cout << "Lower ring Mass:        " << dLowerRingMass << " kg"
         << G4endl;

  const G4double dPTFEpillarMass =
                        m_pPTFEpillarLogicalVolume->GetMass(false, false) / kg;
  G4cout << "PTFE pillars Mass:        " << dPTFEpillarMass << " kg"
         << G4endl;

  const G4double dTPCMass =
                        m_pTpcLogicalVolume->GetMass(false, false) / kg;
  G4cout << "Tpc Mass:        " << dTPCMass << " kg"
         << G4endl;

  const G4double dBottomTPCMass =
                        m_pBottomTpcLogicalVolume->GetMass(false, false) / kg;
  G4cout << "Bottom Tpc Mass:        " << dBottomTPCMass << " kg"
         << G4endl;

  const G4double dFieldShaperRingMass =
                        m_pFieldShaperRingLogicalVolume->GetMass(false, false) / kg;
  G4cout << "Field Shaper Ring Mass:        " << dFieldShaperRingMass << " kg"
         << G4endl;
  
  const G4double dFieldGuardRingMass =
                        m_pFieldGuardLogicalVolume->GetMass(false, false) / kg;
  G4cout << "Field Guard Mass:        " << dFieldGuardRingMass << " kg"
         << G4endl;


  G4cout << "\n";
}


