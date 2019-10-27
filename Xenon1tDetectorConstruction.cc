// XENON Header Files
#include "Xenon1tDetectorConstruction.hh"
#include "Xenon1tDetectorMessenger.hh"
#include "Xenon1tGridParameterisation.hh"
#include "Xenon1tLScintSensitiveDetector.hh"
#include "Xenon1tLXeSensitiveDetector.hh"
#include "Xenon1tMaterials.hh"
#include "Xenon1tPMTsR8520.hh"
#include "Xenon1tTPC.hh"
#include "XenonNtTPC.hh"

// Additional Header Files
#include <algorithm>
#include <cassert>
#include <cmath>
#include <globals.hh>
#include <numeric>
#include <sstream>
#include <vector>

#include <fstream>
#include <iostream>

using std::max;
using std::stringstream;
using std::vector;

// Root Header Files
#include "TFile.h"
#include "TParameter.h"

// G4 Header Files
#include <G4Box.hh>
#include <G4Colour.hh>
#include <G4Cons.hh>
#include <G4Ellipsoid.hh>
#include <G4EllipticalCone.hh>
#include <G4EllipticalTube.hh>
#include <G4ExtrudedSolid.hh>
#include <G4GenericTrap.hh>
#include <G4IntersectionSolid.hh>
#include <G4LogicalVolume.hh>
#include <G4Material.hh>
#include <G4NistManager.hh>
#include <G4OpBoundaryProcess.hh>
#include <G4PVParameterised.hh>
#include <G4PVPlacement.hh>
#include <G4Polycone.hh>
#include <G4Polyhedra.hh>
#include <G4RotationMatrix.hh>
#include <G4SDManager.hh>
#include <G4Sphere.hh>
#include <G4SubtractionSolid.hh>
#include <G4ThreeVector.hh>
#include <G4Torus.hh>
#include <G4Trd.hh>
#include <G4Tubs.hh>
#include <G4UnionSolid.hh>
#include <G4VisAttributes.hh>
#include "G4PhysicalVolumeStore.hh"
#if GEANTVERSION >= 10
#include <G4SystemOfUnits.hh>
#endif

map<G4String, G4double> Xenon1tDetectorConstruction::m_hGeometryParameters;

Xenon1tDetectorConstruction::Xenon1tDetectorConstruction(
    G4String fName, std::string hDetectorname) {
  pCryostatMaterial = "SS316Ti";  //"TiGrade1";
  pMuonVetoMaterial = "GdWater";
  pIBeltCavityMaterial = "Air";
  pTunsgtenPlateHoleMaterial = "GdWater";
  pnVetoConfiguration = "None";

  m_pDetectorMessenger = new Xenon1tDetectorMessenger(this);

  detRootFile = fName;

  pFillBuffer = false;
  pTpcWithBell = true;
  pBottomFillerbool = false;
  pLXeVeto = false;
  pFlagHVFT = false;
  pBeamPipeActive = false;
  pRealTopScreeningMesh = false;
  pRealAnodeMesh = false;
  pRealGateMesh = false;
  pRealCathodeMesh = false;
  pRealBottomScreeningMesh = false;
  pRealS2Mesh = false;
  pRealS2MeshWireDiameter = 1. * mm;

  pBufferThickness = 50. * mm;
  pLXeTopThickness = 304.5 * mm;

  pLXeElectricField = 0.155 * kilovolt / cm;
  pGXeElectricField = 12. * kilovolt / cm;

  m_iVerbosityLevel = 0;

  pNTversion = hDetectorname;

  // XENONnT Related Options
  pTopPMTPatternGeometry = "hexagonal";  // radial or hexagonal
  pLScintVesselThicknessTopBottom = 0.;
  pLScintVesselThicknessSides = 0.;
  pnVeto = false;
}

Xenon1tDetectorConstruction::~Xenon1tDetectorConstruction() {
  delete m_pDetectorMessenger;
}

G4VPhysicalVolume *Xenon1tDetectorConstruction::Construct() {
  Materials = new Xenon1tMaterials();
  Materials->DefineMaterials();

  DefineGeometryParameters();

  ConstructLaboratory();

  G4cout << "Xenon1tDetectorConstruction::Construct() MuonVeto material = "
         << pMuonVetoMaterial << G4endl;
  ConstructMuonVeto();

  G4cout << "Constructing experiment geometry for: " << pNTversion << G4endl;

  ConstructNewSupportStructure();

  if (pNTversion == "XENON1T") {
    ConstructPipe();
  }

  // Construct XENONnT Specific Components
  if (pNTversion == "XENONnT") {
    if (pnVeto) ConstructLScintVessel();  // DR 20160819
    // ConstructVetoAcrylic();  //DR 20161115
    // ConstructWaterDisplacer();  //DR 20160819
  }

  // ConstructCablesPipe();

  G4cout << "Xenon1tDetectorConstruction::Construct() Cryostat material = "
         << pCryostatMaterial << G4endl;
  if (pNTversion == "XENON1T")
    ConstructColumbiaCryostat1T();
  else if (pNTversion == "XENONnT")
    ConstructColumbiaCryostatNT();

  if (pCalibrationSourceSurroundings != "None") {
    G4cout << "Xenon1tDetectorConstruction::Construct() Starting Construction "
              "of Calibration Source"
           << G4endl;

    if (pCalibrationSourceSurroundings == "NeutronGenerator")
      ConstructCalibrationSource("NeutronGenerator");
    else if (pCalibrationSourceSurroundings == "LeadBrick")
      ConstructLeadBrick();
    else if (pCalibrationSourceSurroundings == "BeamPipe") {
      ConstructCalibrationSource("BeamPipe");
      // pBeamPipeActive = true;
      // ConstructCalibrationSource("NeutronGenerator");
    } else if (pCalibrationSourceSurroundings == "PosCollimator" ||
               pCalibrationSourceSurroundings == "IBeltCollimator" ||
               pCalibrationSourceSurroundings == "UBeltCollimator")
      ConstructCalibrationSource(pCalibrationSourceSurroundings);
    else
      G4cout << "Invalid Source Surroundings choice, "
             << "defaulting to None!" << G4endl;
  }

  Xenon1tTPC *pTPC_Constructor_1T;
  XenonNtTPC *pTPC_Constructor_NT;

  if (pNTversion == "XENON1T") {
    pTPC_Constructor_1T = new Xenon1tTPC(
        this, m_pInnerCryostatLogicalVolume, pTPC, pFlagHVFT, pTpcWithBell,
        pLXeVeto, pBottomFillerbool, pRealCathodeMesh, pRealBottomScreeningMesh,
        pRealTopScreeningMesh, pRealAnodeMesh, pRealGateMesh, pRealS2Mesh,
        pLXeTopThickness, m_iVerbosityLevel);
    // Note that this function switches the definition of the mother logical
    // volume to the LXeLogicalVolume
    m_pLXeVacuumVolume = pTPC_Constructor_1T->ConstructTPC(this);
  }

  else if (pNTversion == "XENONnT") {
    pTPC_Constructor_NT = new XenonNtTPC(
        this, m_pInnerCryostatLogicalVolume, pFlagHVFT, pRealCathodeMesh,
        pRealBottomScreeningMesh, pRealTopScreeningMesh, pRealAnodeMesh,
        pRealGateMesh, pRealS2Mesh, pTopPMTPatternGeometry, m_iVerbosityLevel);
    // Note that this function switches the definition of the mother logical
    // volume to the LXeLogicalVolume
    m_pLXeVacuumVolume = pTPC_Constructor_NT->ConstructTPC(this);

    if (pnVetoConfiguration == "Cylinder") {
      G4cout << "nVetoConfiguration: " << pnVetoConfiguration << G4endl;
      ConstructFoilCylinder();
    } else if (pnVetoConfiguration == "Box") {
      G4cout << "nVetoConfiguration: " << pnVetoConfiguration << G4endl;
      ConstructFoilBox();
    } else if (pnVetoConfiguration == "Octagon") {
      G4cout << "nVetoConfiguration: " << pnVetoConfiguration << G4endl;
      ConstructFoilOctagon();
    } else if (pnVetoConfiguration == "None") {
      G4cout << "WARNING!! GdWater nVETO is not constructed!" << G4endl;
    } else {
      G4Exception("XenonDetectorConstruction::Construct()",
                  "DetectorConstruction", FatalException,
                  "Not allowed nVeto configuration. Available ones are: "
                  "Cylinder, Box, Octagon, None");
    }
  }

  ConstructVetoPMTArrays();

  //--- Retrieve Volumes hierarchy and write list to file (Pietro 20180504) ---
  // VolumesHierarchy();
  //
  pTPC_Constructor_NT->PrintGeometryInformation();
  PrintGeometryInformation();
  if (m_iVerbosityLevel >= 1) {
    PrintGeometryInformation();
    if (pNTversion == "XENON1T")
      pTPC_Constructor_1T->PrintGeometryInformation();
    else if (pNTversion == "XENONnT")
      pTPC_Constructor_NT->PrintGeometryInformation();
  }

  if (pCheckOverlap) OverlapCheck();

  MakeDetectorPlots();

  return m_pWorldPhysicalVolume;
}

void Xenon1tDetectorConstruction::DefineGeometryParameters() {
  //========== Laboratory ========== //EDIT PAOLO
  m_hGeometryParameters["LabAxisLength"] = 100. * m;
  m_hGeometryParameters["LabSide"] = 18.479 * m;
  m_hGeometryParameters["LabHeight"] = 20.939 * m;
  m_hGeometryParameters["LabRealHeight"] = 16.065 * m;
  m_hGeometryParameters["ConcreteThickness"] = 0.5 * m;
  m_hGeometryParameters["RockThickness"] = 5. * m;
  m_hGeometryParameters["WorldThickness"] = 10. * m;

  //========== Water tank ==========
  m_hGeometryParameters["WaterTankHeight"] = 10.5 * m;
  m_hGeometryParameters["WaterHeight"] = 10.2 * m;
  m_hGeometryParameters["WaterTankOuterRadius"] = 4.8 * m;
  m_hGeometryParameters["WaterTankThickness"] = 2. * mm;
  m_hGeometryParameters["TankConsR1"] =
      GetGeometryParameter("WaterTankOuterRadius");  // base
  m_hGeometryParameters["TankConsR2"] = 1.20282 * m;
  m_hGeometryParameters["TankConsHeight"] = 1.490 * m;
  m_hGeometryParameters["WaterLineBase"] = 1.923 * m;
  // waterlevel

  m_hGeometryParameters["WaterTankCylinderHeight"] =
      GetGeometryParameter("WaterTankHeight") -
      GetGeometryParameter("TankConsHeight");
  m_hGeometryParameters["WaterTankCylinderInnerHeight"] =
      GetGeometryParameter("WaterTankCylinderHeight") -
      GetGeometryParameter("WaterTankThickness");
  m_hGeometryParameters["WaterTankInnerRadius"] =
      GetGeometryParameter("WaterTankOuterRadius") -
      GetGeometryParameter("WaterTankThickness");

  m_hGeometryParameters["TankOffset"] =
      GetGeometryParameter("WaterHeight") -
      GetGeometryParameter("WaterTankCylinderHeight");

  m_hGeometryParameters["WaterConsR1"] =
      GetGeometryParameter("TankConsR1") -
      GetGeometryParameter("WaterTankThickness");  // base
  m_hGeometryParameters["WaterConsR2"] =
      GetGeometryParameter("TankConsR2") -
      GetGeometryParameter("WaterTankThickness");  // top
  m_hGeometryParameters["WaterConsHeight"] =
      GetGeometryParameter("TankConsHeight") -
      GetGeometryParameter("WaterTankThickness");

  m_hGeometryParameters["AirConsR1"] =
      GetGeometryParameter("WaterLineBase") -
      GetGeometryParameter("WaterTankThickness");  // base
  m_hGeometryParameters["AirConsR2"] =
      GetGeometryParameter("TankConsR2") -
      GetGeometryParameter("WaterTankThickness");  // top
  m_hGeometryParameters["AirConsHeight"] =
      GetGeometryParameter("WaterTankHeight") -
      GetGeometryParameter("WaterHeight") -
      GetGeometryParameter("WaterTankThickness");
  m_hGeometryParameters["AirConsOffset"] =
      GetGeometryParameter("WaterHeight") -
      GetGeometryParameter("WaterTankCylinderHeight");

  //========== Support structure ========== ANDREA T.
  m_hGeometryParameters["x_outer"] = 75. * mm;
  m_hGeometryParameters["y_outer"] = 75. * mm;
  m_hGeometryParameters["z_outer"] = 1582.5 * mm;
  m_hGeometryParameters["x_inner"] = 69. * mm;
  m_hGeometryParameters["y_inner"] = 69. * mm;
  m_hGeometryParameters["z_inner"] = 1582.5 * mm;
  m_hGeometryParameters["z_horizontal_outer"] = 2175. * mm;
  m_hGeometryParameters["z_horizontal_inner"] = 2175. * mm;
  m_hGeometryParameters["z_medium_outer"] = 1532.5 * mm;
  m_hGeometryParameters["z_medium_inner"] = 1532.5 * mm;
  m_hGeometryParameters["x_pos_floor_leg1"] = 2250. * mm;
  m_hGeometryParameters["y_pos_floor_leg1"] = 2250. * mm;
  m_hGeometryParameters["z_pos_floor_leg1"] =
      -(GetGeometryParameter("WaterTankCylinderInnerHeight") / 2. -
        GetGeometryParameter("z_outer"));
  m_hGeometryParameters["x_pos_floor_leg2"] = 2250. * mm;
  m_hGeometryParameters["y_pos_floor_leg2"] = -2250. * mm;
  m_hGeometryParameters["z_pos_floor_leg2"] =
      -(GetGeometryParameter("WaterTankCylinderInnerHeight") / 2. -
        GetGeometryParameter("z_outer"));
  m_hGeometryParameters["x_pos_floor_leg3"] = -2250. * mm;
  m_hGeometryParameters["y_pos_floor_leg3"] = 2250. * mm;
  m_hGeometryParameters["z_pos_floor_leg3"] =
      -(GetGeometryParameter("WaterTankCylinderInnerHeight") / 2. -
        GetGeometryParameter("z_outer"));
  m_hGeometryParameters["x_pos_floor_leg4"] = -2250. * mm;
  m_hGeometryParameters["y_pos_floor_leg4"] = -2250. * mm;
  m_hGeometryParameters["z_pos_floor_leg4"] =
      -(GetGeometryParameter("WaterTankCylinderInnerHeight") / 2. -
        GetGeometryParameter("z_outer"));
  m_hGeometryParameters["z_tilt_leg"] = 2069.5 * mm;

  m_hGeometryParameters["z_platform"] = 745. * mm;
  m_hGeometryParameters["x_platform"] = (1280. + 75. + 75.) * mm;
  m_hGeometryParameters["x_platform1"] =
      (1280. + 75. + 150. + 1490. + 75.) * mm;
  m_hGeometryParameters["R_brace_rods"] = 7. * mm;
  m_hGeometryParameters["R_tie_rods"] = 10. * mm;

  m_hGeometryParameters["y_spreader"] = 60. * mm;
  m_hGeometryParameters["x_spreader"] = 40. * mm;
  m_hGeometryParameters["z_spreader"] = 450. * mm;
  m_hGeometryParameters["z_pos_spreader"] =
      (8310. * mm - GetGeometryParameter("WaterTankCylinderInnerHeight") / 2.) -
      530. * mm;  // MS180209
  m_hGeometryParameters["y_inner_spreader"] = 54. * mm;
  m_hGeometryParameters["x_inner_spreader"] = 34. * mm;

  //========== Pipes ========== ANDREA T.

  // Thickness Central Pipes
  m_hGeometryParameters["Wall_thickness_central_external_pipe"] = 3.2 * mm;
  m_hGeometryParameters["Wall_thickness_central_internal_big_pipe"] = 2. * mm;
  m_hGeometryParameters["Wall_thickness_central_internal_pipe_1"] = 2. * mm;
  m_hGeometryParameters["Wall_thickness_central_internal_pipe_2"] = 3. * mm;
  m_hGeometryParameters["Wall_thickness_central_internal_pipe_3"] = 0.5 * mm;
  m_hGeometryParameters["Wall_thickness_central_internal_pipe_4"] = 1. * mm;
  m_hGeometryParameters["Wall_thickness_central_internal_pipe_5"] = 1. * mm;

  // Radius Central Pipes
  m_hGeometryParameters["Rmax_cylinder_external_central_pipe"] =
      0.5 * 406.6 * mm;
  m_hGeometryParameters["Rmin_cylinder_external_central_pipe"] =
      GetGeometryParameter("Rmax_cylinder_external_central_pipe") -
      GetGeometryParameter("Wall_thickness_central_external_pipe");
  m_hGeometryParameters["Rmax_cylinder_internal_central_big_pipe"] =
      0.5 * 254. * mm;
  m_hGeometryParameters["Rmin_cylinder_internal_central_big_pipe"] =
      GetGeometryParameter("Rmax_cylinder_internal_central_big_pipe") -
      GetGeometryParameter("Wall_thickness_central_internal_big_pipe");
  m_hGeometryParameters["Rmax_cylinder_internal_central_pipe_1"] =
      0.5 * 104. * mm;
  m_hGeometryParameters["Rmin_cylinder_internal_central_pipe_1"] =
      GetGeometryParameter("Rmax_cylinder_internal_central_pipe_1") -
      GetGeometryParameter("Wall_thickness_central_internal_pipe_1");
  m_hGeometryParameters["Rmax_cylinder_internal_central_pipe_2"] =
      0.5 * 48.3 * mm;
  m_hGeometryParameters["Rmin_cylinder_internal_central_pipe_2"] =
      GetGeometryParameter("Rmax_cylinder_internal_central_pipe_2") -
      GetGeometryParameter("Wall_thickness_central_internal_pipe_2");
  m_hGeometryParameters["Rmax_cylinder_internal_central_pipe_3"] =
      0.5 * 18. * mm;
  m_hGeometryParameters["Rmin_cylinder_internal_central_pipe_3"] =
      GetGeometryParameter("Rmax_cylinder_internal_central_pipe_3") -
      GetGeometryParameter("Wall_thickness_central_internal_pipe_3");
  m_hGeometryParameters["Rmax_cylinder_internal_central_pipe_4"] =
      0.5 * 12.7 * mm;
  m_hGeometryParameters["Rmin_cylinder_internal_central_pipe_4"] =
      GetGeometryParameter("Rmax_cylinder_internal_central_pipe_4") -
      GetGeometryParameter("Wall_thickness_central_internal_pipe_4");
  m_hGeometryParameters["Rmax_cylinder_internal_central_pipe_5"] =
      0.5 * 6.35 * mm;
  m_hGeometryParameters["Rmin_cylinder_internal_central_pipe_5"] =
      GetGeometryParameter("Rmax_cylinder_internal_central_pipe_5") -
      GetGeometryParameter("Wall_thickness_central_internal_pipe_5");
  m_hGeometryParameters["R_small_stain"] =
      GetGeometryParameter("Rmax_cylinder_external_central_pipe") + 2. * mm;
  m_hGeometryParameters["height_small_stain"] = 30. * mm;
  m_hGeometryParameters["R_plate"] = 0.5 * 480.0 * mm;
  m_hGeometryParameters["h_plate"] = 0.5 * 6. * mm;
  m_hGeometryParameters["h_plate_low"] = 0.5 * 17. * mm;
  m_hGeometryParameters["y_pipe_box"] = 0.5 * 80. * mm;
  m_hGeometryParameters["z_pipe_box"] = 0.5 * 155. * mm;

  m_hGeometryParameters["y_pipe_box_1"] = 0.5 * 48. * mm;
  m_hGeometryParameters["z_pipe_box_1"] = 0.5 * 60 * mm;
  m_hGeometryParameters["R_cyl_screw_1"] = 0.5 * 47. * mm;
  m_hGeometryParameters["height_cyl_screw_1"] = 0.5 * 10. * mm;
  m_hGeometryParameters["R_cyl_screw_2"] = 0.5 * 26. * mm;
  m_hGeometryParameters["height_cyl_screw_2"] = 0.5 * 16. * mm;
  m_hGeometryParameters["R_min_tolon"] = 0.5 * 258. * mm;
  m_hGeometryParameters["R_max_tolon"] = 0.5 * 310. * mm;
  m_hGeometryParameters["h_tolon"] = 10. * mm;

  m_hGeometryParameters["x_offset_internal_1"] = -45. * mm;
  m_hGeometryParameters["y_offset_internal_1"] = 0. * mm;
  m_hGeometryParameters["x_offset_internal_2"] = 78. * mm;
  m_hGeometryParameters["y_offset_internal_2"] = 0. * mm;
  m_hGeometryParameters["x_offset_internal_3"] = 35. * mm;
  m_hGeometryParameters["y_offset_internal_3"] = -70. * mm;
  m_hGeometryParameters["x_offset_internal_4"] = -10. * mm;
  m_hGeometryParameters["y_offset_internal_4"] = -95. * mm;
  m_hGeometryParameters["x_offset_internal_5"] = 35. * mm;
  m_hGeometryParameters["y_offset_internal_5"] = 70. * mm;

  // Flange Central Pipes
  m_hGeometryParameters["flange_height"] = 0.5 * 80. * mm;
  m_hGeometryParameters["flange_height_internal"] = 0.5 * 5. * mm;
  m_hGeometryParameters["cylinder_height_central_pipe"] =
      0.5 * (435. + GetGeometryParameter("flange_height")) *
      mm;  // 0.5*(490.2*mm+2.*GetGeometryParameter("flange_height"));
  m_hGeometryParameters["cylinder_height_low"] =
      0.5 * 184. *
      mm;  // 0.5*(490.2*mm+2.*GetGeometryParameter("flange_height"));
  // m_hGeometryParameters["torus_height"] = 534.25*mm;
  m_hGeometryParameters["torus_spanned_angle"] = 85. * deg;
  m_hGeometryParameters["torus_radius"] = 373. * mm;
  m_hGeometryParameters["torus_radius_internal_5"] = 443. * mm;
  m_hGeometryParameters["torus_radius_internal_3"] = 303. * mm;
  m_hGeometryParameters["torus_radius_internal_4"] = 278. * mm;
  // m_hGeometryParameters["cylinder_tilted_height_central_pipe"] =
  // 0.5*(640.2*mm+2.*GetGeometryParameter("flange_height"));
  // m_hGeometryParameters["cylinder_tilted_height_central_pipe"] = 0.5*505.*mm;
  m_hGeometryParameters["cylinder_tilted_long_height_central_pipe"] =
      0.5 * 4400. * mm;
  m_hGeometryParameters["flange_radius"] = 0.5 * 560. * mm;
  m_hGeometryParameters["internal_big_flange_radius"] = 0.5 * 305. * mm;
  m_hGeometryParameters["internal_flange_radius_1"] = 0.5 * 134. * mm;
  m_hGeometryParameters["internal_flange_radius_2"] = 0.5 * 72. * mm;
  m_hGeometryParameters["internal_flange_radius_3"] = 0.5 * 44. * mm;
  m_hGeometryParameters["internal_flange_radius_4"] = 0.5 * 40. * mm;
  m_hGeometryParameters["internal_flange_radius_5"] = 0.5 * 34. * mm;

  // Small Pipes
  m_hGeometryParameters["Wall_thickness_small_pipe"] = 1.5 * mm;
  m_hGeometryParameters["Rmax_cylinder_small_pipe"] = 0.5 * 38.1 * mm;
  m_hGeometryParameters["Rmin_cylinder_small_pipe"] =
      GetGeometryParameter("Rmax_cylinder_small_pipe") -
      GetGeometryParameter("Wall_thickness_small_pipe");

  m_hGeometryParameters["flange_height_small_pipe"] = 0.5 * 27. * mm;
  m_hGeometryParameters["flange_radius_small_pipe"] = 0.5 * 125.6 * mm;
  m_hGeometryParameters["cylinder_height_small_pipe"] =
      0.5 * (435. + GetGeometryParameter("flange_height_small_pipe")) * mm;
  m_hGeometryParameters["cylinder_long_height_small_pipe"] = 0.5 * (4600.) * mm;
  m_hGeometryParameters["torus_radius_small_pipe"] = 373. * mm;

  //========== Cables Pipe ==========
  m_hGeometryParameters["CablesPipeBaseThickness"] = 5. * mm;
  m_hGeometryParameters["CablesPipeBaseOuterRadius"] = (400. / 2.) * mm;
  m_hGeometryParameters["CablesPipeBaseInnerRadius"] =
      GetGeometryParameter("CablesPipeBaseOuterRadius") -
      GetGeometryParameter("CablesPipeBaseThickness");
  m_hGeometryParameters["CablesPipeBaseHeight"] = (1514. / 2.) * mm;

  m_hGeometryParameters["CablePipe_tilt_angle"] = 95. * deg;
  m_hGeometryParameters["CablesPipeThickness"] = 5. * mm;
  m_hGeometryParameters["CablesPipeOuterRadius"] = (400. / 2.) * mm;
  m_hGeometryParameters["CablesPipeInnerRadius"] =
      GetGeometryParameter("CablesPipeOuterRadius") -
      GetGeometryParameter("CablesPipeThickness");
  //	m_hGeometryParameters["CablesPipeHeight"] =
  // GetGeometryParameter("WaterTankInnerRadius") -
  // GetGeometryParameter("CablesPipeBaseOuterRadius") -1.67*cm;
  m_hGeometryParameters["CablesPipeHeight"] =
      (GetGeometryParameter("WaterTankInnerRadius") -
       GetGeometryParameter("CablesPipeBaseOuterRadius") -
       2. * GetGeometryParameter("CablesPipeOuterRadius") *
           sin(GetGeometryParameter("CablePipe_tilt_angle") - 90. * deg)) /
      cos(GetGeometryParameter("CablePipe_tilt_angle") - 90. * deg);

  //========== (Columbia) Cryostat ==========
  if (pCryostatMaterial != "SS316Ti")
    G4Exception("XenonDetectorConstruction::DefineGeometryParameters()",
                "DetectorConstruction", FatalException,
                "Bad Cryostat material: it must be SS316Ti");

  // Shell Thickness
  m_hGeometryParameters["OuterCryostatThickness"] = 5.00 * mm;
  m_hGeometryParameters["OuterCryostatThicknessTop"] = 5.00 * mm;
  m_hGeometryParameters["OuterCryostatThicknessBot"] = 5.00 * mm;
  m_hGeometryParameters["InnerCryostatThickness"] = 5.00 * mm;
  m_hGeometryParameters["InnerCryostatThicknessTop"] = 5.00 * mm;
  m_hGeometryParameters["InnerCryostatThicknessBot"] = 5.00 * mm;

  //========== Z offsets ========== PIETRO (Nov 2017)
  // Needed to get correct Z positions of electrodes and PMT Windows as
  // measured after TPC assembly (see here
  // https://xe1t-wiki.lngs.infn.it/doku.php?id=xenon:xenon1t:digangi:tpc-z-pos-mc)
  const G4double dZOffset_Anode_TopScreeningMesh_1 = -4.38 * mm;
  const G4double dTopMeshRingHeight_OffsetZ = 0.85 * mm;
  const G4double dZOffset_Anode_TopScreeningMesh =
      dZOffset_Anode_TopScreeningMesh_1 + dTopMeshRingHeight_OffsetZ;
  m_hGeometryParameters["TopMeshRingHeight_OffsetZ"] =
      dTopMeshRingHeight_OffsetZ;
  m_hGeometryParameters["ZOffset_Anode_TopScreeningMesh"] =
      dZOffset_Anode_TopScreeningMesh;

  //========== Water tank shifting ==========
  m_hGeometryParameters["TankOffsetX"] = 2360 * mm;  // placing on a side
  m_hGeometryParameters["TankOffsetZ"] = -469.3 * mm;
  // MS Jan2016, to set Z=0 at the gate mesh level
  // UPDATE Pietro (Nov 2017), added offset after changing
  // anode-topscreeningmesh distance
  // DR 20180921 - Back to Marco's value for the 'TankOffsetZ'
  //               (before this commit wrongly called 'GlobalOffsetZ').
  //               Pietro's modification inserted in 'RockOffsetZ',
  //               to shift the geometry globally.

  if (pNTversion == "XENON1T") {
    // rock shifting to set Z=0 at gate mesh level
    m_hGeometryParameters["RockOffsetZ"] = 91. * mm
        + GetGeometryParameter("ZOffset_Anode_TopScreeningMesh");

    // outer vessel geometry
    m_hGeometryParameters["OuterCryostatOuterDiameter"] = 1630. * mm;
    m_hGeometryParameters["OuterCryostatCylinderHeight"] = 1687. * mm;
    m_hGeometryParameters["OuterCryostatR0top"] = 1309. * mm;
    m_hGeometryParameters["OuterCryostatR1top"] = 256. * mm;
    m_hGeometryParameters["OuterCryostatR0bot"] = 1309. * mm;
    m_hGeometryParameters["OuterCryostatR1bot"] = 256. * mm;

    // flange between top and bottom of outer vessel
    m_hGeometryParameters["OuterCryostatFlangeHeight"] = 90. * mm;
    m_hGeometryParameters["OuterCryostatFlangeZ"] = 781. * mm;
    m_hGeometryParameters["OuterCryostatFlangeThickness"] = 100. * mm;
    m_hGeometryParameters["OuterCryostatOffsetZ"] = 530. * mm;
    // taken from an email by A.Tiseni and Rob, 20 Feb 2014
    // (platform and Outer Cryo separated by 533 mm along Z)

    // inner vessel geometry
    m_hGeometryParameters["InnerCryostatOuterDiameter"] = 1110. * mm;
    m_hGeometryParameters["InnerCryostatCylinderHeight"] = 1420. * mm;
    m_hGeometryParameters["InnerCryostatR0top"] = 893. * mm;
    m_hGeometryParameters["InnerCryostatR1top"] = 176. * mm;
    m_hGeometryParameters["InnerCryostatR0bot"] = 893. * mm;
    m_hGeometryParameters["InnerCryostatR1bot"] = 176. * mm;

    // flange between top and bottom of inner vessel
    m_hGeometryParameters["InnerCryostatFlangeHeight"] = 90. * mm;
    m_hGeometryParameters["InnerCryostatFlangeZ"] = 614. * mm;
    m_hGeometryParameters["InnerCryostatFlangeThickness"] = 62.5 * mm;
    m_hGeometryParameters["InnerCryostatOffsetZ"] = 96. * mm;
  }

  else if (pNTversion == "XENONnT") {
    // rock shifting to set Z=0 at gate mesh level
    // Last change: DR 20181029
    m_hGeometryParameters["RockOffsetZ"] = 126.0905 * mm;

    // outer vessel geometry
    m_hGeometryParameters["OuterCryostatOuterDiameter"] = 1630. * mm;
    m_hGeometryParameters["OuterCryostatCylinderHeight"] = 2065. * mm;
    m_hGeometryParameters["OuterCryostatR0top"] = 1309. * mm;
    m_hGeometryParameters["OuterCryostatR1top"] = 256.02 * mm;
    m_hGeometryParameters["OuterCryostatR0bot"] = 1309. * mm;
    m_hGeometryParameters["OuterCryostatR1bot"] = 256.02 * mm;
    m_hGeometryParameters["OuterCryostatFlangeHeight"] = 90. * mm;
    m_hGeometryParameters["OuterCryostatFlangeThickness"] = 100. * mm;
    m_hGeometryParameters["OuterCryostatRingsHeight"] = 5. * mm;
    m_hGeometryParameters["OuterCryostatRingsThickness"] = 80. * mm;
    m_hGeometryParameters["OuterCryostatCylinderBaseToRing1BotZ"] = 700. * mm;
    m_hGeometryParameters["OuterCryostatRing1TopToRing2BotZ"] = 460. * mm;
    m_hGeometryParameters["OuterCryostatRing2TopToFlangeBotZ"] = 785. * mm;
    m_hGeometryParameters["OuterCryostatOffsetZ"] = 343.5 * mm;
    if (pnVeto)
      m_hGeometryParameters["z_nVetoOffset"] = 594.09775 * mm + 50.5 * mm;
    else
      m_hGeometryParameters["z_nVetoOffset"] = 0.;

    // inner vessel geometry
    m_hGeometryParameters["InnerCryostatOuterDiameter"] = 1470. * mm;
    m_hGeometryParameters["InnerCryostatCylinderHeight"] = 1895.921 * mm;
    m_hGeometryParameters["InnerCryostatR0top"] = 1205. * mm;
    m_hGeometryParameters["InnerCryostatR1top"] = 225. * mm;
    m_hGeometryParameters["InnerCryostatR0bot"] = 1205. * mm;
    m_hGeometryParameters["InnerCryostatR1bot"] = 225. * mm;
    m_hGeometryParameters["InnerCryostatFlangeHeight"] = 90. * mm;
    m_hGeometryParameters["InnerCryostatFlangeThickness"] = 60. * mm;
    m_hGeometryParameters["InnerCryostatRingsHeight"] = 5. * mm;
    m_hGeometryParameters["InnerCryostatRingsThickness"] = 40. * mm;
    m_hGeometryParameters["InnerCryostatCylinderBaseToRing1BotZ"] = 611.338 * mm;
    m_hGeometryParameters["InnerCryostatRing1TopToRing2BotZ"] = 580. * mm;
    m_hGeometryParameters["InnerCryostatRing2TopToFlangeBotZ"] = 581.903 * mm;
    m_hGeometryParameters["InnerCryostatOffsetZ"] = -40.284 * mm;
  }

  //========== LScint Vessel ==========
  // DR 20160819 - LScint vessel geometry
  m_hGeometryParameters["LScintVesselTolerance"] = 1. * cm;
  // radial distance from the outer cryostat flange (to allow leveling)
  m_hGeometryParameters["LScintVesselThickness"] = 2.54 * cm;
  m_hGeometryParameters["LScintVesselHeight"] = 3801. * mm;  // FA 20171110
  m_hGeometryParameters["LScintVesselThicknessTopBottom"] =
      pLScintVesselThicknessTopBottom;
  m_hGeometryParameters["LScintVesselThicknessSides"] =
      pLScintVesselThicknessSides;
  m_hGeometryParameters["LScintVesselInnerRadius"] =
      0.5 * GetGeometryParameter("OuterCryostatOuterDiameter") +
      GetGeometryParameter("OuterCryostatFlangeThickness") +
      GetGeometryParameter("LScintVesselTolerance");
  m_hGeometryParameters["LScintVesselOuterRadius"] =
      GetGeometryParameter("LScintVesselInnerRadius") +
      GetGeometryParameter("LScintVesselThicknessSides") +
      2 * GetGeometryParameter("LScintVesselThickness");

  m_hGeometryParameters["WaterDisplacerCylinderRadius"] =
      GetGeometryParameter("LScintVesselInnerRadius");
  m_hGeometryParameters["WaterDisplacerCylinderHeight"] =
      GetGeometryParameter("LScintVesselHeight");
  m_hGeometryParameters["LScintVetoCylinderRadius"] =
      GetGeometryParameter("LScintVesselOuterRadius");
  m_hGeometryParameters["LScintVetoCylinderHeight"] =
      GetGeometryParameter("WaterDisplacerCylinderHeight") +
      GetGeometryParameter("LScintVesselThicknessTopBottom");
  m_hGeometryParameters["VetoPipesFeedthroughRadius"] = 315. * mm;
  m_hGeometryParameters["VetoChainFeedthroughRadius"] = 126. * mm;

  //========== HVFT ==========
  m_hGeometryParameters["HVFT_angular_Offset"] = 4.80587 * radian;
  m_hGeometryParameters["HVFT_x_Offset"] = -450.333 * mm;
  m_hGeometryParameters["HVFT_y_Offset"] = -260.0 * mm;
  m_hGeometryParameters["HVFT_OuterSS_Radius"] = 12.7 * mm;
  m_hGeometryParameters["HVFT_InnerPoly_Radius"] = 11.049 * mm;
  m_hGeometryParameters["HVFT_InnerPoly_Height"] = 92.964 * mm;
  m_hGeometryParameters["HVFT_BottInnSS_Radius"] = 6. * mm;
  m_hGeometryParameters["HVFT_lastBottSS_Radius"] = 6.35 * mm;
  m_hGeometryParameters["lastSShvft_height"] = 16.97 * mm;

  m_hGeometryParameters["HVFTdistanceFromInnerCryostat"] = 5. * mm;

  //========== Lead Brick ========== Andrew 22/08/12
  m_hGeometryParameters["LeadBrick_len_x"] = 10.0 * cm;
  m_hGeometryParameters["LeadBrick_len_y"] = 14.7 * cm;
  m_hGeometryParameters["LeadBrick_len_z"] = 10.0 * cm;

  m_hGeometryParameters["LeadBrick_pos_y"] =
      650 * mm + GetGeometryParameter("LeadBrick_len_y") / 2;
  m_hGeometryParameters["LeadBrick_pos_x"] = 0 * mm;
  m_hGeometryParameters["LeadBrick_pos_z"] = 0 * mm;

  //========== Calibration Source Positioning ==========
  // Andrew 22/08/12
  // Shayne 2014/02/25
  // Jacques 2016/04/18
  G4double Source_X = pCalSourcePosition.x();
  G4double Source_Y = pCalSourcePosition.y();
  G4double Source_Z = pCalSourcePosition.z();
  m_hGeometryParameters["Source_x"] = Source_X * mm;
  m_hGeometryParameters["Source_y"] = Source_Y * mm;
  m_hGeometryParameters["Source_z"] = Source_Z * mm;

  //========== Beam Pipe ==========
  // Jacques 2016/04/18
  m_hGeometryParameters["BeamPipeLength"] = 4.07 * m;
  m_hGeometryParameters["BeamPipeOffset_z"] = 450. * mm;
  m_hGeometryParameters["BeamPipeDeclination"] = 25. * deg;
  m_hGeometryParameters["BeamPipeAzimuth"] = 50. * deg;
  m_hGeometryParameters["GeneratorContainer_oD"] = 138. * mm;
  m_hGeometryParameters["NGRegionOffset_z"] = 52 * mm;

  // Positions
  /*m_hGeometryParameters["GeneratrContainer_distance"] = 40. *mm;
  m_hGeometryParameters["GeneratorContainer_x"] = 0 *mm;
  //Outside diameter of cryostat plus radius of neutron generator
  m_hGeometryParameters["GeneratorContainer_y"] =
    GetGeometryParameter("OuterCryostatOuterDiameter")/2
    +
  GetGeometryParameter("GeneratorContainer_oD")/2+GetGeometryParameter("GeneratorContainer_distance");
    m_hGeometryParameters["GeneratorContainer_z"] = -552. *mm;*/

  //========== TPC PMTs ==========
  if (pNTversion == "XENON1T") {
    m_hGeometryParameters["NbOfTopPMTs"] = 127;
    m_hGeometryParameters["NbOfBottomPMTs"] = 121;
  } else if (pNTversion == "XENONnT") {
    m_hGeometryParameters["NbOfBottomPMTs"] = 241;
    if (pTopPMTPatternGeometry == "radial")
      m_hGeometryParameters["NbOfTopPMTs"] = 225;
    else if (pTopPMTPatternGeometry == "hexagonal")
      m_hGeometryParameters["NbOfTopPMTs"] = 253;
  }
  m_hGeometryParameters["NbOfPMTs"] = GetGeometryParameter("NbOfTopPMTs") +
                                      GetGeometryParameter("NbOfBottomPMTs");

  //========== Total Numer of PMTs ==========
  m_hGeometryParameters["NbTopPMTs"] = GetGeometryParameter("NbOfTopPMTs");
  m_hGeometryParameters["NbBottomPMTs"] =
      GetGeometryParameter("NbOfBottomPMTs");
  m_hGeometryParameters["NbLSPMTs"] = 0;
  m_hGeometryParameters["NbLSTopPMTs"] = 0;
  m_hGeometryParameters["NbLSBottomPMTs"] = 0;
  m_hGeometryParameters["NbLSSidePMTs"] = 0;
  m_hGeometryParameters["NbLSSidePMTColumns"] = 0;
  m_hGeometryParameters["NbLSSidePMTRows"] = 0;

  // nVeto PMTs
  if (pNTversion == "XENONnT"){
    if (pnVetoConfiguration!="None"){
      m_hGeometryParameters["NbLSPMTs"] = 120;
      m_hGeometryParameters["NbLSSidePMTs"] = 120;

      if (pnVetoConfiguration == "Cylinder"){
        m_hGeometryParameters["NbLSSidePMTColumns"] = 15;
        m_hGeometryParameters["NbLSSidePMTRows"] = 8;
      } else if (pnVetoConfiguration == "Box") {
        m_hGeometryParameters["NbLSSidePMTColumns"] = 20;
        m_hGeometryParameters["NbLSSidePMTRows"] = 6;
      } else if (pnVetoConfiguration == "Octagon") {
        m_hGeometryParameters["NbLSSidePMTColumns"] = 20;
        m_hGeometryParameters["NbLSSidePMTColumns_SideAlongSS"] = 3;
        m_hGeometryParameters["NbLSSidePMTColumns_DiagonalSide"] = 2;
        m_hGeometryParameters["NbLSSidePMTRows"] = 6;
      }
    }
  }

  if (pnVeto) {
    m_hGeometryParameters["NbLSPMTs"] = 120;
    m_hGeometryParameters["NbLSSidePMTs"] = 120;
    m_hGeometryParameters["NbLSSidePMTColumns"] = 15;
    m_hGeometryParameters["NbLSSidePMTRows"] = 8;
  }
  m_hGeometryParameters["NbWaterPMTs"] = 84;
  m_hGeometryParameters["NbWaterTopPMTs"] = 24;
  m_hGeometryParameters["NbWaterBottomPMTs"] = 24;
  m_hGeometryParameters["NbWaterSidePMTs"] = 36;
  m_hGeometryParameters["NbWaterSidePMTColumns"] = 12;
  m_hGeometryParameters["NbWaterSidePMTRows"] = 3;
  m_hGeometryParameters["NbPMTs"] = GetGeometryParameter("NbOfTopPMTs") +
                                    GetGeometryParameter("NbOfBottomPMTs") +
                                    GetGeometryParameter("NbLSPMTs") +
                                    GetGeometryParameter("NbWaterPMTs");

  // Version 3 of LXe veto, using Andreas drawing - Cyril 2013/11/05
  m_hGeometryParameters["NbBottomLXeVetoPMTs"] = 48;
  m_hGeometryParameters["NbTopLXeVetoPMTs"] = 48;
  m_hGeometryParameters["NbBelowLXeVetoPMTs"] = 32;
  m_hGeometryParameters["NbAboveLXeVetoPMTs"] = 32;
  m_hGeometryParameters["NbCenterLXeVetoPMTs"] = 0;
  m_hGeometryParameters["NbLXeVetoPMTs"] =
      GetGeometryParameter("NbBottomLXeVetoPMTs") +
      GetGeometryParameter("NbTopLXeVetoPMTs") +
      GetGeometryParameter("NbBelowLXeVetoPMTs") +
      GetGeometryParameter("NbAboveLXeVetoPMTs") +
      GetGeometryParameter("NbCenterLXeVetoPMTs");

  //========== Geometry 8" PMTs (Hamamatsu R5912-100-10-Y001) ==========
  // Pietro 190708
  m_hGeometryParameters["PMTWindowOuterRadius"]       = 101.0 * mm;
  m_hGeometryParameters["PMTWindowOuterHalfZ"]        = 79.*mm;
  m_hGeometryParameters["PMTWindowTopZ"]              = 71.*mm;
  m_hGeometryParameters["PMTPhotocathodeOuterRadius"] = 99.5 * mm;
  m_hGeometryParameters["PMTPhotocathodeOuterHalfZ"]  = 78.*mm;
  m_hGeometryParameters["PMTPhotocathodeTopZ"]        = -25.*mm;
  m_hGeometryParameters["PMTPhotocathodeInnerRadius"] = 99.0 * mm;
  m_hGeometryParameters["PMTPhotocathodeInnerHalfZ"]  = 77.5*mm;
  m_hGeometryParameters["PMTBodyOuterRadius"]         = 47.5*mm;
  m_hGeometryParameters["PMTBodyInnerRadius"]         = 46.5*mm;
  m_hGeometryParameters["PMTBodyHeight"]              = 62.*mm;
  m_hGeometryParameters["PMTBaseOuterRadius"]         = 58.*mm;
  m_hGeometryParameters["PMTBaseInnerRadius"]         = 57.*mm;
  m_hGeometryParameters["PMTBaseHeight"]              = 73.*mm;
  //"PMTBaseHeight" should be 85 mm (reduced for the moment to avoid overlap)
  m_hGeometryParameters["PMTBaseInteriorHeight"]      = 71.*mm;

  //========== Veto PMTs position ==========
  m_hGeometryParameters["LSTopPMTWindowZ"] = 195. * cm;
  m_hGeometryParameters["LSBottomPMTWindowZ"] = -195. * cm;
  m_hGeometryParameters["LSSidePMTWindowR"] = 172.5 * cm;
  m_hGeometryParameters["LSSidePMTWindowX"] = 195. * cm;
  m_hGeometryParameters["LSTopPMTDistance"] = 80. * cm;
  m_hGeometryParameters["LSBottomPMTDistance"] = 80. * cm;
  m_hGeometryParameters["LSSidePMTRowDistance"] = 50. * cm;
  m_hGeometryParameters["NVetoPMTTopRowZ"] = 550. * mm;
  m_hGeometryParameters["NVetoPMTBottomRowZ"] = -2000. * mm;
  // m_hGeometryParameters["WaterTopPMTWindowZ"] = 49.25*cm;
  m_hGeometryParameters["WaterTopPMTWindowZ"] = 429.75 * cm;
  m_hGeometryParameters["WaterBottomPMTWindowZ"] = -429.75 * cm;
  m_hGeometryParameters["WaterSidePMTWindowR"] = 450. * cm; // was 450 cm
  m_hGeometryParameters["WaterSidePMTRowDistance"] = 214.875 * cm;


  //========== Meshes ==========
  m_hGeometryParameters["GridMeshThickness"] = 0.2 * mm;
  m_hGeometryParameters["TopScreeningMeshThickness"] = 0.2 * mm;
  m_hGeometryParameters["BottomScreeningMeshThickness"] = 0.2 * mm;
  m_hGeometryParameters["CathodeMeshThickness"] = 0.2 * mm;
  m_hGeometryParameters["AnodeMeshThickness"] = 0.2 * mm;
  m_hGeometryParameters["GateMeshThickness"] = 0.2 * mm;

  //========== Real meshes activated by messenger function ==========
  m_hGeometryParameters["TopScreeningGridWireDiameter"] = 0.178 * mm;
  m_hGeometryParameters["TopScreeningGridWirePitch"] = 10.2 * mm;
  m_hGeometryParameters["BottomScreeningGridWireDiameter"] = 0.216 * mm;
  m_hGeometryParameters["BottomScreeningGridWirePitch"] = 7.75 * mm;
  m_hGeometryParameters["GateGridWireDiameter"] = 0.127 * mm;
  m_hGeometryParameters["GateGridWirePitch"] =
      3.5 * mm + GetGeometryParameter("GateGridWireDiameter");
  m_hGeometryParameters["AnodeGridWireDiameter"] = 0.178 * mm;
  m_hGeometryParameters["AnodeGridWirePitch"] =
      3.5 * mm + GetGeometryParameter("GateGridWireDiameter");
  m_hGeometryParameters["CathodeGridWireDiameter"] = 0.216 * mm;
  m_hGeometryParameters["CathodeGridWirePitch"] = 7.75 * mm;
  m_hGeometryParameters["RealS2GridWireDiameter"] = pRealS2MeshWireDiameter;

  //========== Neutron Veto Reflector ===========
  m_hGeometryParameters["FoilThickness"] = 1.5*mm;
  m_hGeometryParameters["FoilOffset"] = 10.*mm;
  //========== nVeto cylinder ===========
  m_hGeometryParameters["FoilCylinderInnerRadius"] =
    GetGeometryParameter("LSSidePMTWindowR")
    + GetGeometryParameter("PMTWindowTopZ")
    + GetGeometryParameter("PMTBodyHeight")
    + GetGeometryParameter("PMTBaseHeight")
    + GetGeometryParameter("FoilOffset");
  m_hGeometryParameters["FoilCylinderOuterRadius"] =
    GetGeometryParameter("FoilCylinderInnerRadius")
    + GetGeometryParameter("FoilThickness");
  m_hGeometryParameters["FoilCylinderHeight"] =
    GetGeometryParameter("LSTopPMTWindowZ")
    + std::fabs(GetGeometryParameter("LSBottomPMTWindowZ")) + 10.*cm;
  m_hGeometryParameters["FoilCylinderLowerSideHeight"] = 643.*mm;
  //========== nVeto box ===========
  m_hGeometryParameters["FoilSidePanelWidth"] = 2. * GetGeometryParameter("LSSidePMTWindowX")
    + 140.*mm;
  m_hGeometryParameters["FoilSidePanelHeight"] =
    GetGeometryParameter("LSTopPMTWindowZ")
    + std::fabs(GetGeometryParameter("LSBottomPMTWindowZ")) - 62.*cm;
  //========== nVeto octagon ===========
  m_hGeometryParameters["FoilBackwardOffset"] = 10. * mm; // Reflective foil moved backwards wrt PMT window center
  m_hGeometryParameters["FoilOctagonSideLength"] =
    2. * (GetGeometryParameter("LSSidePMTWindowX") + GetGeometryParameter("FoilBackwardOffset"));
  m_hGeometryParameters["FoilOctagonSidePanelWidth"] = 0.5 * GetGeometryParameter("FoilOctagonSideLength") / 1.207; // regular octagon
  m_hGeometryParameters["FoilOctagonSidePanelHeight"] =
    GetGeometryParameter("LSTopPMTWindowZ")
    + std::fabs(GetGeometryParameter("LSBottomPMTWindowZ")) - 62.*cm;
  m_hGeometryParameters["DistanceBetweenColumns_SideAlongSS"] = 0.73 * GetGeometryParameter("FoilOctagonSidePanelWidth") * 0.5 * mm;
  m_hGeometryParameters["DistanceBetweenColumns_DiagonalSide"] = 0.38 * GetGeometryParameter("FoilOctagonSidePanelWidth") * mm;
}

G4double Xenon1tDetectorConstruction::GetGeometryParameter(
    const char *szParameter) {
  if (m_hGeometryParameters.find(szParameter) != m_hGeometryParameters.end()) {
    return m_hGeometryParameters[szParameter];
  } else {
    G4cout << "----> Parameter " << szParameter << " is not defined!!!!!"
           << G4endl;
    return 0;
  }
}

void Xenon1tDetectorConstruction::ConstructLaboratory()  // EDIT PAOLO
{
  const G4double dLabHalfAxis = 0.5 * GetGeometryParameter("LabAxisLength");
  const G4double dLabHalfSide = 0.5 * GetGeometryParameter("LabSide");
  const G4double dLabHalfHeight = 0.5 * GetGeometryParameter("LabHeight");

  const G4double dConcreteHalfAxis =
      dLabHalfAxis + GetGeometryParameter("ConcreteThickness");
  const G4double dConcreteHalfSide =
      dLabHalfSide + GetGeometryParameter("ConcreteThickness");
  const G4double dConcreteHalfHeight =
      dLabHalfHeight + GetGeometryParameter("ConcreteThickness");

  const G4double dRockHalfAxis =
      dConcreteHalfAxis + GetGeometryParameter("RockThickness");
  const G4double dRockHalfSide =
      dConcreteHalfSide + GetGeometryParameter("RockThickness");
  const G4double dRockHalfHeight =
      dConcreteHalfHeight + GetGeometryParameter("RockThickness");

  const G4double dWorldHalfAxis =
      dConcreteHalfAxis + GetGeometryParameter("WorldThickness");
  const G4double dWorldHalfSide =
      dRockHalfHeight + GetGeometryParameter("WorldThickness");

  const G4double dRockOffsetZ = GetGeometryParameter("RockOffsetZ");
  const G4double dRockOffsetX = GetGeometryParameter("TankOffsetX");

  G4Material *Air = G4Material::GetMaterial("G4_AIR");
  G4Material *GSrock = G4Material::GetMaterial("GSrock");
  G4Material *Concrete = G4Material::GetMaterial("Concrete");

  G4Box *solidWorld =
      new G4Box("sWorld", dWorldHalfSide, dWorldHalfAxis, dWorldHalfSide);
  m_pWorldLogicalVolume =
      new G4LogicalVolume(solidWorld, Air, "WorldVolume", 0, 0, 0);
  m_pWorldPhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(), m_pWorldLogicalVolume, "World", 0, false, 0);
  m_pWorldLogicalVolume->SetVisAttributes(G4VisAttributes::Invisible);

  G4EllipticalTube *solidETubRock = new G4EllipticalTube(
      "sETubRock", dRockHalfSide, dRockHalfHeight, dRockHalfAxis);
  m_pRockLogicalVolume =
      new G4LogicalVolume(solidETubRock, GSrock, "RockLogicalVolume", 0, 0, 0);
  G4RotationMatrix *pRotationMatrixRock = new G4RotationMatrix();
  pRotationMatrixRock->rotateX(90. * deg);
  m_pRockPhysicalVolume = new G4PVPlacement(
      pRotationMatrixRock, G4ThreeVector(-dRockOffsetX, 0, dRockOffsetZ),
      m_pRockLogicalVolume, "Rock", m_pWorldLogicalVolume, false, 0);
  m_pRockLogicalVolume->SetVisAttributes(G4VisAttributes::Invisible);

  const G4double dFloorDistance =
      GetGeometryParameter("LabRealHeight") - dLabHalfHeight;
  const G4double dConcreteDistance =
      dFloorDistance + GetGeometryParameter("ConcreteThickness");
  const G4double dConcreteBoxHalfWidth =
      0.5 * (dConcreteHalfHeight - dConcreteDistance);

  G4EllipticalTube *solidETubConcrete =
      new G4EllipticalTube("sETubConcrete", dConcreteHalfSide,
                           dConcreteHalfHeight, dConcreteHalfAxis);
  G4Box *solidBoxConcrete = new G4Box("sBoxConcrete", dConcreteHalfSide,
                                      dConcreteBoxHalfWidth, dConcreteHalfAxis);
  G4SubtractionSolid *solidSubConcrete = new G4SubtractionSolid(
      "sSubConcrete", solidETubConcrete, solidBoxConcrete, 0,
      G4ThreeVector(0, dConcreteDistance + dConcreteBoxHalfWidth, 0));
  m_pConcreteLogicalVolume = new G4LogicalVolume(solidSubConcrete, Concrete,
                                                 "ConcreteLogicalVolume", 0, 0, 0);
  m_pConcretePhysicalVolume =
      new G4PVPlacement(0, G4ThreeVector(0, 0, 0), m_pConcreteLogicalVolume,
                        "Concrete", m_pRockLogicalVolume, false, 0);
  m_pConcreteLogicalVolume->SetVisAttributes(G4VisAttributes::Invisible);

  const G4double dLabBoxHalfWidth = 0.5 * (dLabHalfHeight - dFloorDistance);

  G4EllipticalTube *solidETubLab = new G4EllipticalTube(
      "sETubLab", dLabHalfSide, dLabHalfHeight, dLabHalfAxis);
  G4Box *solidBoxLab =
      new G4Box("sBoxLab", dLabHalfSide, dLabBoxHalfWidth, dLabHalfAxis);
  G4SubtractionSolid *solidSubLab = new G4SubtractionSolid(
      "sSubLab", solidETubLab, solidBoxLab, 0,
      G4ThreeVector(0, dFloorDistance + dLabBoxHalfWidth, 0));
  m_pLabLogicalVolume =
      new G4LogicalVolume(solidSubLab, Air, "LabLogicalVolume", 0, 0, 0);
  m_pLabPhysicalVolume =
      new G4PVPlacement(0, G4ThreeVector(0, 0, 0), m_pLabLogicalVolume, "Lab",
                        m_pConcreteLogicalVolume, false, 0);
  m_pLabLogicalVolume->SetVisAttributes(G4VisAttributes::Invisible);
}

void Xenon1tDetectorConstruction::ConstructMuonVeto() {
  //========== Water Tank ==========
  const G4double dWaterTankCylinderHalfZ =
      0.5 * GetGeometryParameter("WaterTankCylinderHeight");
  const G4double dWaterTankRadius =
      GetGeometryParameter("WaterTankOuterRadius");

  const G4double dTankConsR1 = GetGeometryParameter("TankConsR1");
  const G4double dTankConsR2 = GetGeometryParameter("TankConsR2");
  const G4double dTankConsZ = 0.5 * GetGeometryParameter("TankConsHeight");

  const G4double dWaterTankOffsetZ = -1090.3 * mm;
  // DR 20180920 - Hard-coded from 1T geometry. Originally optimized for
  //               cryostat placement, which now (1T + nT) doesn't make sense.

  const G4double dWaterConeOffsetZ =
      dWaterTankOffsetZ + dWaterTankCylinderHalfZ +
      0.5 * GetGeometryParameter("TankConsHeight");
  const G4double dTankOffsetX = GetGeometryParameter("TankOffsetX");

  G4RotationMatrix *pRotationMatrixTank = new G4RotationMatrix();
  pRotationMatrixTank->rotateX(-90. * deg);

  G4Material *SS304LSteel = G4Material::GetMaterial("SS304LSteel");

  G4Tubs *pWaterTankTubs =
      new G4Tubs("WaterTankTubs", 0. * cm, dWaterTankRadius,
                 dWaterTankCylinderHalfZ, 0. * deg, 360. * deg);

  G4Cons *pTankCons = new G4Cons("TankCons", 0., dTankConsR1, 0., dTankConsR2,
                                 dTankConsZ, 0. * deg, 360. * deg);

  // Tank cylinder
  m_pWaterTankTubLogicalVolume = new G4LogicalVolume(
      pWaterTankTubs, SS304LSteel, "WaterTankTubeLogicalVolume", 0, 0, 0);

  m_pWaterTankTubePhysicalVolume = new G4PVPlacement(
      pRotationMatrixTank, G4ThreeVector(dTankOffsetX, -dWaterTankOffsetZ, 0),
      m_pWaterTankTubLogicalVolume, "SS_WaterTankTube", m_pLabLogicalVolume,
      false, 0);

  // Tank cone
  m_pTankConsLogicalVolume =
      new G4LogicalVolume(pTankCons, SS304LSteel, "WaterTankConeLogicalVolume",
                          0, 0, 0);

  m_pTankConsPhysicalVolume = new G4PVPlacement(
      pRotationMatrixTank, G4ThreeVector(dTankOffsetX, -dWaterConeOffsetZ, 0),
      m_pTankConsLogicalVolume, "SS_WaterTankCone", m_pLabLogicalVolume,
      false, 0);

  //========== Water ==========
  const G4double dWaterCylinderHalfZ =
      0.5 * GetGeometryParameter("WaterTankCylinderInnerHeight");
  const G4double dWaterRadius = GetGeometryParameter("WaterTankInnerRadius");
  const G4double dWaterOffsetZ =
      0.5 * GetGeometryParameter("WaterTankThickness");

  const G4double dWaterConsR1 = GetGeometryParameter("WaterConsR1");
  const G4double dWaterConsR2 = GetGeometryParameter("WaterConsR2");
  const G4double dWaterConsZ = 0.5 * GetGeometryParameter("WaterConsHeight");

  const G4double dAirConsR1 = GetGeometryParameter("AirConsR1");
  const G4double dAirConsR2 = GetGeometryParameter("AirConsR2");
  const G4double dAirConsZ = 0.5 * GetGeometryParameter("AirConsHeight");

  const G4double dAirConeOffsetZ = 0.5 * GetGeometryParameter("AirConsOffset");

  G4Material *Air = G4Material::GetMaterial("G4_AIR");

  G4Material *muonvetoMaterial;
  // check input material, is it a valid one ?
  if (pMuonVetoMaterial) {
    if (pMuonVetoMaterial == "Water" || pMuonVetoMaterial == "GdWater" || pMuonVetoMaterial == "LScint" ||
        pMuonVetoMaterial == "Gd_LScint" || pMuonVetoMaterial == "B_LScint" ||
        pMuonVetoMaterial == "G4_AIR") {
      muonvetoMaterial = G4Material::GetMaterial(pMuonVetoMaterial);
    } else {
      G4Exception("XenonDetectorConstruction::ConstructMuonVeto()",
                  "DetectorConstruction", FatalException,
                  "Not allowed Material for the Muon Veto, good ones are: "
                  "Water, GdWater, LScint, Gd_LScint, B_LScint, G4_AIR");
    }
  }

  G4Tubs *pWaterTubs = new G4Tubs("WaterTubs", 0. * cm, dWaterRadius,
                                  dWaterCylinderHalfZ, 0. * deg, 360. * deg);

  G4Cons *pWaterCons =
      new G4Cons("WaterCons", 0., dWaterConsR1, 0., dWaterConsR2, dWaterConsZ,
                 0. * deg, 360. * deg);
  G4Cons *pAirCons = new G4Cons("AirCons", 0., dAirConsR1, 0., dAirConsR2,
                                dAirConsZ, 0. * deg, 360. * deg);

  m_pWaterLogicalVolume =
      new G4LogicalVolume(pWaterTubs, muonvetoMaterial, "Water_TubeLogicalVolume",
                          0, 0, 0);

  m_pWaterPhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(0, 0, dWaterOffsetZ), m_pWaterLogicalVolume, "Water_Tube",
      m_pWaterTankTubLogicalVolume, false, 0);

  m_pWaterConsLogicalVolume =
      new G4LogicalVolume(pWaterCons, muonvetoMaterial, "Water_ConeLogicalVolume", 0, 0, 0);
  m_pWaterConsPhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(0, 0, -dWaterOffsetZ), m_pWaterConsLogicalVolume,
      "Water_Cone", m_pTankConsLogicalVolume, false, 0);

  m_pAirConsLogicalVolume =
      new G4LogicalVolume(pAirCons, Air, "Air_ConeLogical", 0, 0, 0);
  m_pAirConsPhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(0, 0, dAirConeOffsetZ), m_pAirConsLogicalVolume,
      "Air_Cone", m_pWaterConsLogicalVolume, false, 0);

  //----- Optical surface -----
  G4OpticalSurface *OpWaterSurface = new G4OpticalSurface("WaterSurface");
  OpWaterSurface->SetType(dielectric_metal);  //(dielectric_LUT)
  OpWaterSurface->SetModel(unified);          //(LUT);
  OpWaterSurface->SetFinish(polished);        //(polishedvm2000air);
                                              // Specular Reflector

  // Serena
  const G4int NUM = 2;
  G4double pp[NUM] = {1. * eV, 5. * eV};
  G4double reflectivity_W[NUM] = {0.99, 0.99};  //{0.95, 0.95};
  G4MaterialPropertiesTable *SMPT_W = new G4MaterialPropertiesTable();
  SMPT_W->AddProperty("REFLECTIVITY", pp, reflectivity_W, NUM);
  OpWaterSurface->SetMaterialPropertiesTable(SMPT_W);

  G4LogicalBorderSurface *OuterWaterSurface = new G4LogicalBorderSurface(
      "OuterWaterSurface", m_pWaterPhysicalVolume,
      m_pWaterTankTubePhysicalVolume, OpWaterSurface);  // SERENA REPLACEMENT
  if (m_iVerbosityLevel >= 1) {
    if (OuterWaterSurface->GetVolume1() == m_pWaterPhysicalVolume)
      G4cout << "Outer Vol 1 equal to Water" << G4endl;
    if (OuterWaterSurface->GetVolume2() == m_pWaterTankTubePhysicalVolume)
      G4cout << "Outer Vol 2 equal to Tank" << G4endl;  // SERENA REPLACEMENT
  }

  //==== Optical surface ====
  // SERENA
  // Applying wall reflectivity into the water/tank interface
  G4OpticalSurface *OpWater2Surface = new G4OpticalSurface("Water2Surface");
  OpWater2Surface->SetType(dielectric_metal);  //(dielectric_LUT)
  OpWater2Surface->SetModel(unified);          //(LUT);
  OpWater2Surface->SetFinish(polished);        //(polishedvm2000air);

  G4double reflectivity_W2[NUM] = {0.99, 0.99};  //{0.90, 0.90};
  G4MaterialPropertiesTable *SMPT_W2 = new G4MaterialPropertiesTable();
  SMPT_W2->AddProperty("REFLECTIVITY", pp, reflectivity_W2, NUM);
  OpWater2Surface->SetMaterialPropertiesTable(SMPT_W2);

  G4LogicalBorderSurface *OuterWater2Surface = new G4LogicalBorderSurface(
      "OuterWater2Surface", m_pWaterConsPhysicalVolume,
      m_pTankConsPhysicalVolume, OpWater2Surface);

  if (m_iVerbosityLevel >= 1) {
    if (OuterWater2Surface->GetVolume1() == m_pWaterConsPhysicalVolume)
      G4cout << "Outer Vol 1 equal to Water Cons" << G4endl;
    if (OuterWater2Surface->GetVolume2() == m_pTankConsPhysicalVolume)
      G4cout << "Outer Vol 2 equal to Tank Cons"
             << G4endl;  // SERENA REPLACEMENT
  }

  G4OpticalSurface *OpAirSurface = new G4OpticalSurface("AirSurface");
  OpAirSurface->SetType(dielectric_metal);  //(dielectric_LUT)
  OpAirSurface->SetModel(unified);          //(LUT);
  OpAirSurface->SetFinish(polished);        //(polishedvm2000air);

  G4double reflectivity_A[NUM] = {0.99, 0.99};  //{0.90, 0.90};
  G4MaterialPropertiesTable *SMPT_A = new G4MaterialPropertiesTable();
  SMPT_A->AddProperty("REFLECTIVITY", pp, reflectivity_A, NUM);
  OpAirSurface->SetMaterialPropertiesTable(SMPT_A);

  G4LogicalBorderSurface *OuterAirSurface = new G4LogicalBorderSurface(
      "OuterAirSurface", m_pAirConsPhysicalVolume, m_pTankConsPhysicalVolume,
      OpAirSurface);  // SERENA REPLACEMENT

  if (m_iVerbosityLevel >= 1) {
    if (OuterAirSurface->GetVolume1() == m_pAirConsPhysicalVolume)
      G4cout << "Outer Vol 1 equal to Air Cons" << G4endl;
    if (OuterAirSurface->GetVolume2() == m_pTankConsPhysicalVolume)
      G4cout << "Outer Vol 2 equal to Tank Celling"
             << G4endl;  // SERENA REPLACEMENT
  }

  //==== attributes ====
  G4Colour hWaterTankColor(0.500, 0.500, 0.500, 0.1);
  G4VisAttributes *pWaterTankVisAtt = new G4VisAttributes(hWaterTankColor);
  pWaterTankVisAtt->SetVisibility(false);
  m_pWaterTankTubLogicalVolume->SetVisAttributes(pWaterTankVisAtt);
  m_pTankConsLogicalVolume->SetVisAttributes(pWaterTankVisAtt);

  G4Colour hWaterColor(0, 0, 1.);
  G4VisAttributes *pWaterVisAtt = new G4VisAttributes(hWaterColor);
  pWaterVisAtt->SetVisibility(false);
  m_pWaterLogicalVolume->SetVisAttributes(pWaterVisAtt);
  m_pWaterConsLogicalVolume->SetVisAttributes(pWaterVisAtt);

  G4Colour hAirColor(0, 1., 1.);
  G4VisAttributes *pAirVisAtt = new G4VisAttributes(hAirColor);
  pAirVisAtt->SetVisibility(false);
  m_pAirConsLogicalVolume->SetVisAttributes(pAirVisAtt);
}

// Function used to construct a beam
G4UnionSolid *Xenon1tDetectorConstruction::ConstructBeam(
    G4double x_outer, G4double y_outer, G4double z_outer, G4double x_inner,
    G4double y_inner, G4double z_inner) {
  // Andrea Tiseni 21-12-2012

  G4Box *outer_box = new G4Box("outer_box", x_outer, y_outer, z_outer);
  G4Box *inner_box = new G4Box("inner_box", x_inner, y_inner, z_inner);
  G4SubtractionSolid *leg =
      new G4SubtractionSolid("leg1", outer_box, inner_box);
  G4UnionSolid *real_leg;
  real_leg = (G4UnionSolid *)leg;
  return real_leg;
}

void Xenon1tDetectorConstruction::ConstructNewSupportStructure() {
  // Andrea Tiseni 21-12-2012

  // material used to construct the beams
  G4Material *SS304LSteel = G4Material::GetMaterial("SS304LSteel");
  G4Material *Air = G4Material::GetMaterial("G4_AIR");

  G4double cryo_offset = 530. * mm;
  // MS180209 hard-coded to the OuterCryostatOffsetZ
  // of the 1T Cryostat (also for the nT version)

  // some constants
  G4double dWaterCylinderZ =
      GetGeometryParameter("WaterTankCylinderInnerHeight");
  G4double dWaterConsZed = GetGeometryParameter("WaterConsHeight");

  G4double x_outer = GetGeometryParameter("x_outer");
  G4double y_outer = GetGeometryParameter("y_outer");
  G4double z_outer = GetGeometryParameter("z_outer");
  G4double x_inner = GetGeometryParameter("x_inner");
  G4double y_inner = GetGeometryParameter("y_inner");
  G4double z_inner = GetGeometryParameter("z_inner");
  G4double z_medium_outer = GetGeometryParameter("z_medium_outer");
  G4double z_medium_inner = GetGeometryParameter("z_medium_inner");
  G4double x_pos_floor_leg1 = GetGeometryParameter("x_pos_floor_leg1");
  G4double y_pos_floor_leg1 = GetGeometryParameter("y_pos_floor_leg1");
  G4double z_pos_floor_leg1 = GetGeometryParameter("z_pos_floor_leg1");
  G4double x_pos_floor_leg2 = GetGeometryParameter("x_pos_floor_leg2");
  G4double y_pos_floor_leg2 = GetGeometryParameter("y_pos_floor_leg2");
  G4double z_pos_floor_leg2 = GetGeometryParameter("z_pos_floor_leg2");
  G4double x_pos_floor_leg3 = GetGeometryParameter("x_pos_floor_leg3");
  G4double y_pos_floor_leg3 = GetGeometryParameter("y_pos_floor_leg3");
  G4double z_pos_floor_leg3 = GetGeometryParameter("z_pos_floor_leg3");
  G4double x_pos_floor_leg4 = GetGeometryParameter("x_pos_floor_leg4");
  G4double y_pos_floor_leg4 = GetGeometryParameter("y_pos_floor_leg4");
  G4double z_pos_floor_leg4 = GetGeometryParameter("z_pos_floor_leg4");
  G4double z_horizontal_outer = GetGeometryParameter("z_horizontal_outer");
  G4double z_horizontal_inner = GetGeometryParameter("z_horizontal_inner");
  G4double z_tilt_beam = GetGeometryParameter("z_tilt_leg");

  G4double z_platform = GetGeometryParameter("z_platform");
  G4double x_platform = GetGeometryParameter("x_platform");
  G4double x_platform1 = GetGeometryParameter("x_platform1");
  G4double pi_g = 3.141592653589793;

  G4double x_spreader = GetGeometryParameter("x_spreader");
  G4double y_spreader = GetGeometryParameter("y_spreader");
  G4double z_spreader = GetGeometryParameter("z_spreader");
  G4double x_inner_spreader = GetGeometryParameter("x_inner_spreader");
  G4double y_inner_spreader = GetGeometryParameter("y_inner_spreader");
  G4double z_pos_spreader =
      GetGeometryParameter("z_pos_spreader");  // MS180209 -cryo_offset;
  if (m_iVerbosityLevel >= 1)
    G4cout << ">>>>>> Cryo Offset for the spreader " << cryo_offset << G4endl;

  // angles for the tilted beams
  G4double tilt_angle = 28. * deg;
  G4double costilt = cos(tilt_angle);
  G4double sintilt = sin(tilt_angle);
  G4double rotation_angle = 45. * deg;
  G4double cosrot = cos(rotation_angle);
  G4double sinrot = sin(rotation_angle);

  // zed measure of the tilt leg in the cylinder
  G4double z_tilt_low_beam =
      (dWaterCylinderZ - z_outer * 2. - z_medium_outer * 2. - x_outer -
       2 * x_outer * sintilt) /
      (2. * costilt);

  // zed measure of the tilt leg in the cons
  G4double z_tilt_cons_beam =
      z_tilt_beam - z_tilt_low_beam - (x_outer / costilt) * sintilt;

  z_tilt_beam = z_tilt_low_beam;

  // zed measure of the connection between vertical beam and the tilted beams
  G4double z_connection = x_outer / 2.;

  // construction of the legs
  G4UnionSolid *pLegFloor1Volume =
      ConstructBeam(x_outer, y_outer, z_outer, x_inner, y_inner, z_inner);
  G4UnionSolid *pLegMediumVolume = ConstructBeam(
      x_outer, y_outer, z_medium_outer, x_inner, y_inner, z_medium_inner);
  G4UnionSolid *pLegHorizontalVolume =
      ConstructBeam(x_outer, y_outer, z_horizontal_outer, x_inner, y_inner,
                    z_horizontal_inner);
  G4UnionSolid *pLegTiltedVolume = ConstructBeam(x_outer, y_outer, z_tilt_beam,
                                                 x_inner, y_inner, z_tilt_beam);
  G4UnionSolid *pLegTiltedConsVolume = ConstructBeam(
      x_outer, y_outer, z_tilt_cons_beam, x_inner, y_inner, z_tilt_cons_beam);
  G4UnionSolid *pLegTiltedCons1Volume = ConstructBeam(
      x_outer, y_outer, z_tilt_cons_beam, x_inner, y_inner, z_tilt_cons_beam);

  G4UnionSolid *pLegConnection = ConstructBeam(x_outer, y_outer, z_connection,
                                               x_inner, y_inner, z_connection);
  G4UnionSolid *pLegPlatformVolume =
      ConstructBeam(x_outer, y_outer, z_platform, x_inner, y_inner, z_platform);
  G4UnionSolid *pLegSpreader =
      ConstructBeam(x_spreader, y_spreader, z_spreader, x_inner_spreader,
                    y_inner_spreader, z_spreader);

  G4double x_center_tilt_beam = -z_tilt_beam * sintilt * cosrot;
  G4double y_center_tilt_beam = -z_tilt_beam * sintilt * sinrot;

  G4double x_air = GetGeometryParameter("x_inner");
  G4double y_air = GetGeometryParameter("y_inner");

  G4Box *air_beam_floor = new G4Box("box_1", x_air, y_air, z_outer);
  G4Box *air_beam_medium = new G4Box("box_2", x_air, y_air, z_medium_outer);
  G4Box *air_beam_connection = new G4Box("box_3", x_air, y_air, z_connection);
  G4Box *air_beam_horizontal =
      new G4Box("box_4", x_air, y_air, z_horizontal_outer);
  G4Box *air_beam_tilted = new G4Box("box_5", x_air, y_air, z_tilt_beam);
  G4Box *air_beam_tilted_cons =
      new G4Box("box_6", x_air, y_air, z_tilt_cons_beam);
  G4Box *air_beam_platform = new G4Box("box_7", x_air, y_air, z_platform);

  G4Box *air_beam_spreader =
      new G4Box("box_7", x_inner_spreader, y_inner_spreader, z_spreader);

  G4double z_triangle = 112.5 * mm;

  G4int nCVtx = 8;
  std::vector<G4TwoVector> cvtx(nCVtx);
  cvtx[0] = G4TwoVector(-130, -5);
  cvtx[1] = G4TwoVector(-130, 5);
  cvtx[2] = G4TwoVector(130, 5);
  cvtx[3] = G4TwoVector(130, -5);
  cvtx[4] = G4TwoVector(-5, -5);
  cvtx[5] = G4TwoVector(-5, 5);
  cvtx[6] = G4TwoVector(5, 5);
  cvtx[7] = G4TwoVector(5, -5);

  G4GenericTrap *pand = new G4GenericTrap("lsl", z_triangle, cvtx);

  // rotation matrix
  G4RotationMatrix *Rot = new G4RotationMatrix;
  Rot->rotateZ(90. * deg);
  Rot->rotateX(90. * deg);
  Rot->rotateY(90. * deg);
  G4RotationMatrix *Rot1 = new G4RotationMatrix;
  Rot1->rotateX(90. * deg);
  Rot1->rotateY(90. * deg);
  G4RotationMatrix *Rot2 = new G4RotationMatrix;
  Rot2->rotateZ(-rotation_angle);
  Rot2->rotateY(tilt_angle);
  G4RotationMatrix *Rot3 = new G4RotationMatrix;
  Rot3->rotateZ(rotation_angle);
  Rot3->rotateY(tilt_angle);
  G4RotationMatrix *Rot4 = new G4RotationMatrix;
  Rot4->rotateZ(rotation_angle);
  Rot4->rotateY(360. * deg - tilt_angle);
  G4RotationMatrix *Rot5 = new G4RotationMatrix;
  Rot5->rotateZ(-rotation_angle);
  Rot5->rotateY(360. * deg - tilt_angle);

  G4RotationMatrix *Rot6 = new G4RotationMatrix;
  Rot6->rotateZ(-rotation_angle);
  Rot6->rotateY(tilt_angle);

  // placement of the floor leg and connection between vertical and tilted beams

  m_pLegFloor1LogicalVolume =
      new G4LogicalVolume(pLegFloor1Volume, SS304LSteel, "leg1Logical");
  m_pLegFloor1PhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(x_pos_floor_leg1, y_pos_floor_leg1, z_pos_floor_leg1),
      m_pLegFloor1LogicalVolume, "support_leg1floor_physical",
      m_pWaterLogicalVolume, false, 0);
  m_pLegConnection1LogicalVolume = new G4LogicalVolume(
      pLegConnection, SS304LSteel, "leg1_connection_Logical");
  m_pLegConnection1PhysicalVolume = new G4PVPlacement(
      0,
      G4ThreeVector(
          x_pos_floor_leg1, y_pos_floor_leg1,
          z_pos_floor_leg1 + z_outer + z_medium_outer * 2. + z_connection),
      m_pLegConnection1LogicalVolume, "support_legconnection1_physical",
      m_pWaterLogicalVolume, false, 0);
  m_pLegConnection2PhysicalVolume = new G4PVPlacement(
      0,
      G4ThreeVector(
          x_pos_floor_leg2, y_pos_floor_leg2,
          z_pos_floor_leg1 + z_outer + z_medium_outer * 2. + z_connection),
      m_pLegConnection1LogicalVolume, "support_legconnection2_physical",
      m_pWaterLogicalVolume, false, 0);
  m_pLegConnection3PhysicalVolume = new G4PVPlacement(
      0,
      G4ThreeVector(
          x_pos_floor_leg3, y_pos_floor_leg3,
          z_pos_floor_leg1 + z_outer + z_medium_outer * 2. + z_connection),
      m_pLegConnection1LogicalVolume, "support_legconnection3_physical",
      m_pWaterLogicalVolume, false, 0);
  m_pLegConnection4PhysicalVolume = new G4PVPlacement(
      0,
      G4ThreeVector(
          x_pos_floor_leg4, y_pos_floor_leg4,
          z_pos_floor_leg1 + z_outer + z_medium_outer * 2. + z_connection),
      m_pLegConnection1LogicalVolume, "support_legconnection4_physical",
      m_pWaterLogicalVolume, false, 0);
  m_pLegFloor2PhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(x_pos_floor_leg2, y_pos_floor_leg2, z_pos_floor_leg2),
      m_pLegFloor1LogicalVolume, "support_leg2floor_physical",
      m_pWaterLogicalVolume, false, 0);
  m_pLegFloor3PhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(x_pos_floor_leg3, y_pos_floor_leg3, z_pos_floor_leg3),
      m_pLegFloor1LogicalVolume, "support_leg3floor_physical",
      m_pWaterLogicalVolume, false, 0);
  m_pLegFloor4PhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(x_pos_floor_leg4, y_pos_floor_leg4, z_pos_floor_leg4),
      m_pLegFloor1LogicalVolume, "support_leg4floor_physical",
      m_pWaterLogicalVolume, false, 0);

  // Placement of Air leg floor

  m_pLegFloorAir1LogicalVolume =
      new G4LogicalVolume(air_beam_floor, Air, "air_leg1logical");
  m_pLegFloorAir1PhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(x_pos_floor_leg1, y_pos_floor_leg1, z_pos_floor_leg1),
      m_pLegFloorAir1LogicalVolume, "Air_support_leg1floor_physical",
      m_pWaterLogicalVolume, false, 0);
  m_pLegFloorAir2PhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(x_pos_floor_leg2, y_pos_floor_leg2, z_pos_floor_leg2),
      m_pLegFloorAir1LogicalVolume, "Air_support_leg2floor_physical",
      m_pWaterLogicalVolume, false, 0);
  m_pLegFloorAir3PhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(x_pos_floor_leg3, y_pos_floor_leg3, z_pos_floor_leg3),
      m_pLegFloorAir1LogicalVolume, "Air_support_leg3floor_physical",
      m_pWaterLogicalVolume, false, 0);
  m_pLegFloorAir4PhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(x_pos_floor_leg4, y_pos_floor_leg4, z_pos_floor_leg4),
      m_pLegFloorAir1LogicalVolume, "Air_support_leg4floor_physical",
      m_pWaterLogicalVolume, false, 0);

  // Placement of Air leg connection
  m_pLegConnectionAir1LogicalVolume = new G4LogicalVolume(
      air_beam_connection, Air, "air_leg1_connection_Logical");
  m_pLegConnectionAir1PhysicalVolume = new G4PVPlacement(
      0,
      G4ThreeVector(
          x_pos_floor_leg1, y_pos_floor_leg1,
          z_pos_floor_leg1 + z_outer + z_medium_outer * 2. + z_connection),
      m_pLegConnectionAir1LogicalVolume, "Air_support_legconnection1_physical",
      m_pWaterLogicalVolume, false, 0);
  m_pLegConnectionAir2PhysicalVolume = new G4PVPlacement(
      0,
      G4ThreeVector(
          x_pos_floor_leg2, y_pos_floor_leg2,
          z_pos_floor_leg1 + z_outer + z_medium_outer * 2. + z_connection),
      m_pLegConnectionAir1LogicalVolume, "Air_support_legconnection2_physical",
      m_pWaterLogicalVolume, false, 0);
  m_pLegConnectionAir3PhysicalVolume = new G4PVPlacement(
      0,
      G4ThreeVector(
          x_pos_floor_leg3, y_pos_floor_leg3,
          z_pos_floor_leg1 + z_outer + z_medium_outer * 2. + z_connection),
      m_pLegConnectionAir1LogicalVolume, "Air_support_legconncection3_physical",
      m_pWaterLogicalVolume, false, 0);
  m_pLegConnectionAir4PhysicalVolume = new G4PVPlacement(
      0,
      G4ThreeVector(
          x_pos_floor_leg4, y_pos_floor_leg4,
          z_pos_floor_leg1 + z_outer + z_medium_outer * 2. + z_connection),
      m_pLegConnectionAir1LogicalVolume, "Air_support_legconnection4_physical",
      m_pWaterLogicalVolume, false, 0);

  // placement of tilted legs

  G4double x_pos_tilt_beam = x_pos_floor_leg1 - z_tilt_beam * sintilt * cosrot;
  G4double y_pos_tilt_beam = y_pos_floor_leg1 - z_tilt_beam * sintilt * sinrot;
  G4double z_pos_tilt_beam = z_pos_floor_leg1 + z_outer + z_medium_outer * 2. +
                             z_tilt_beam * costilt + x_outer +
                             x_outer * sintilt;

  m_pLegTiltedLogicalVolume =
      new G4LogicalVolume(pLegTiltedVolume, SS304LSteel, "leg1Logical");
  m_pLegTilted1PhysicalVolume = new G4PVPlacement(
      Rot6, G4ThreeVector(x_pos_tilt_beam, y_pos_tilt_beam, z_pos_tilt_beam),
      m_pLegTiltedLogicalVolume, "support_leg1_tilted_physical",
      m_pWaterLogicalVolume, false, 0);
  m_pLegTilted2PhysicalVolume = new G4PVPlacement(
      Rot4, G4ThreeVector(-x_pos_tilt_beam, y_pos_tilt_beam, z_pos_tilt_beam),
      m_pLegTiltedLogicalVolume, "support_leg2_tilted_physical",
      m_pWaterLogicalVolume, false, 0);
  m_pLegTilted3PhysicalVolume = new G4PVPlacement(
      Rot3, G4ThreeVector(x_pos_tilt_beam, -y_pos_tilt_beam, z_pos_tilt_beam),
      m_pLegTiltedLogicalVolume, "support_leg3_tilted_physical",
      m_pWaterLogicalVolume, false, 0);
  m_pLegTilted4PhysicalVolume = new G4PVPlacement(
      Rot5, G4ThreeVector(-x_pos_tilt_beam, -y_pos_tilt_beam, z_pos_tilt_beam),
      m_pLegTiltedLogicalVolume, "support_leg4_tilted_physical",
      m_pWaterLogicalVolume, false, 0);

  // Placement of Tilted Legs Air
  m_pLegTiltedAirLogicalVolume =
      new G4LogicalVolume(air_beam_tilted, Air, "air_leg1Logical");
  m_pLegTiltedAir1PhysicalVolume = new G4PVPlacement(
      Rot6, G4ThreeVector(x_pos_tilt_beam, y_pos_tilt_beam, z_pos_tilt_beam),
      m_pLegTiltedAirLogicalVolume, "Air_support_leg1_tilted_physical",
      m_pWaterLogicalVolume, false, 0);
  m_pLegTiltedAir2PhysicalVolume = new G4PVPlacement(
      Rot4, G4ThreeVector(-x_pos_tilt_beam, y_pos_tilt_beam, z_pos_tilt_beam),
      m_pLegTiltedAirLogicalVolume, "Air_support_leg2_tilted_physical",
      m_pWaterLogicalVolume, false, 0);
  m_pLegTiltedAir3PhysicalVolume = new G4PVPlacement(
      Rot3, G4ThreeVector(x_pos_tilt_beam, -y_pos_tilt_beam, z_pos_tilt_beam),
      m_pLegTiltedAirLogicalVolume, "Air_support_leg3_tilted_physical",
      m_pWaterLogicalVolume, false, 0);
  m_pLegTiltedAir4PhysicalVolume = new G4PVPlacement(
      Rot5, G4ThreeVector(-x_pos_tilt_beam, -y_pos_tilt_beam, z_pos_tilt_beam),
      m_pLegTiltedAirLogicalVolume, "Air_support_leg4_tilted_physical",
      m_pWaterLogicalVolume, false, 0);

  // Placement of Medium Vertical Volume

  m_pLegMedium1LogicalVolume =
      new G4LogicalVolume(pLegMediumVolume, SS304LSteel, "legMedium1Logical");
  m_pLegMedium1PhysicalVolume = new G4PVPlacement(
      0,
      G4ThreeVector(x_pos_floor_leg1, y_pos_floor_leg1,
                    z_pos_floor_leg1 + z_outer + z_medium_outer),
      m_pLegMedium1LogicalVolume, "support_leg1_medium_physical",
      m_pWaterLogicalVolume, false, 0);
  m_pLegMedium2LogicalVolume =
      new G4LogicalVolume(pLegMediumVolume, SS304LSteel, "legMedium2Logical");
  m_pLegMedium2PhysicalVolume = new G4PVPlacement(
      0,
      G4ThreeVector(x_pos_floor_leg2, y_pos_floor_leg2,
                    z_pos_floor_leg1 + z_outer + z_medium_outer),
      m_pLegMedium2LogicalVolume, "support_leg2_medium_physical",
      m_pWaterLogicalVolume, false, 0);
  m_pLegMedium3LogicalVolume =
      new G4LogicalVolume(pLegMediumVolume, SS304LSteel, "legmedium3Logical");
  m_pLegMedium3PhysicalVolume = new G4PVPlacement(
      0,
      G4ThreeVector(x_pos_floor_leg3, y_pos_floor_leg3,
                    z_pos_floor_leg1 + z_outer + z_medium_outer),
      m_pLegMedium3LogicalVolume, "support_leg3_medium_physical",
      m_pWaterLogicalVolume, false, 0);
  m_pLegMedium4LogicalVolume =
      new G4LogicalVolume(pLegMediumVolume, SS304LSteel, "legMedium4Logical");
  m_pLegMedium4PhysicalVolume = new G4PVPlacement(
      0,
      G4ThreeVector(x_pos_floor_leg4, y_pos_floor_leg4,
                    z_pos_floor_leg1 + z_outer + z_medium_outer),
      m_pLegMedium4LogicalVolume, "support_leg4_medium_physical",
      m_pWaterLogicalVolume, false, 0);

  // medium volume air
  m_pLegMediumAir1LogicalVolume =
      new G4LogicalVolume(air_beam_medium, Air, "Air_legMedium1Logical");
  m_pLegMediumAir1PhysicalVolume = new G4PVPlacement(
      0,
      G4ThreeVector(x_pos_floor_leg1, y_pos_floor_leg1,
                    z_pos_floor_leg1 + z_outer + z_medium_outer),
      m_pLegMediumAir1LogicalVolume, "Air_support_leg1_medium_physical",
      m_pWaterLogicalVolume, false, 0);
  m_pLegMediumAir2PhysicalVolume = new G4PVPlacement(
      0,
      G4ThreeVector(x_pos_floor_leg2, y_pos_floor_leg2,
                    z_pos_floor_leg1 + z_outer + z_medium_outer),
      m_pLegMediumAir1LogicalVolume, "Air_support_leg2_medium_physical",
      m_pWaterLogicalVolume, false, 0);
  m_pLegMediumAir3PhysicalVolume = new G4PVPlacement(
      0,
      G4ThreeVector(x_pos_floor_leg3, y_pos_floor_leg3,
                    z_pos_floor_leg1 + z_outer + z_medium_outer),
      m_pLegMediumAir1LogicalVolume, "Air_support_leg3_medium_physical",
      m_pWaterLogicalVolume, false, 0);
  m_pLegMediumAir4PhysicalVolume = new G4PVPlacement(
      0,
      G4ThreeVector(x_pos_floor_leg4, y_pos_floor_leg4,
                    z_pos_floor_leg1 + z_outer + z_medium_outer),
      m_pLegMediumAir1LogicalVolume, "Air_support_leg4_medium_physical",
      m_pWaterLogicalVolume, false, 0);

  // Placement of horizontal volume

  m_pLegHorizontal1LogicalVolume = new G4LogicalVolume(
      pLegHorizontalVolume, SS304LSteel, "leg1horizontalLogical");
  m_pLegHorizontal1PhysicalVolume = new G4PVPlacement(
      Rot,
      G4ThreeVector(x_pos_floor_leg1,
                    y_pos_floor_leg1 - x_outer - z_horizontal_outer,
                    z_pos_floor_leg1 + z_outer),
      m_pLegHorizontal1LogicalVolume, "support_leg1_horizontal_physical",
      m_pWaterLogicalVolume, false, 0);

  m_pLegHorizontal2PhysicalVolume = new G4PVPlacement(
      Rot1,
      G4ThreeVector(x_pos_floor_leg2 - x_outer - z_horizontal_outer,
                    y_pos_floor_leg2, z_pos_floor_leg1 + z_outer),
      m_pLegHorizontal1LogicalVolume, "support_leg2_horizontal_physical",
      m_pWaterLogicalVolume, false, 0);

  m_pLegHorizontal3PhysicalVolume = new G4PVPlacement(
      Rot,
      G4ThreeVector(x_pos_floor_leg3,
                    y_pos_floor_leg3 - x_outer - z_horizontal_outer,
                    z_pos_floor_leg1 + z_outer),
      m_pLegHorizontal1LogicalVolume, "support_leg3_horizontal_physical",
      m_pWaterLogicalVolume, false, 0);

  m_pLegHorizontal4PhysicalVolume = new G4PVPlacement(
      Rot1,
      G4ThreeVector(x_pos_floor_leg1 - x_outer - z_horizontal_outer,
                    y_pos_floor_leg1, z_pos_floor_leg1 + z_outer),
      m_pLegHorizontal1LogicalVolume, "support_leg4_horizontal_physical",
      m_pWaterLogicalVolume, false, 0);

  m_pLegHorizontal5PhysicalVolume = new G4PVPlacement(
      Rot,
      G4ThreeVector(x_pos_floor_leg1,
                    y_pos_floor_leg1 - x_outer - z_horizontal_outer,
                    z_pos_floor_leg1 + z_outer + z_medium_outer * 2.),
      m_pLegHorizontal1LogicalVolume, "support_leg5_horizontal_physical",
      m_pWaterLogicalVolume, false, 0);

  m_pLegHorizontal6PhysicalVolume = new G4PVPlacement(
      Rot1,
      G4ThreeVector(x_pos_floor_leg2 - x_outer - z_horizontal_outer,
                    y_pos_floor_leg2,
                    z_pos_floor_leg1 + z_outer + z_medium_outer * 2.),
      m_pLegHorizontal1LogicalVolume, "support_leg6_horizontal_physical",
      m_pWaterLogicalVolume, false, 0);

  m_pLegHorizontal7PhysicalVolume = new G4PVPlacement(
      Rot,
      G4ThreeVector(x_pos_floor_leg3,
                    y_pos_floor_leg3 - x_outer - z_horizontal_outer,
                    z_pos_floor_leg1 + z_outer + z_medium_outer * 2.),
      m_pLegHorizontal1LogicalVolume, "support_leg7_horizontal_physical",
      m_pWaterLogicalVolume, false, 0);

  m_pLegHorizontal8PhysicalVolume = new G4PVPlacement(
      Rot1,
      G4ThreeVector(x_pos_floor_leg1 - x_outer - z_horizontal_outer,
                    y_pos_floor_leg1,
                    z_pos_floor_leg1 + z_outer + z_medium_outer * 2.),
      m_pLegHorizontal1LogicalVolume, "support_leg8_horizontal_physical",
      m_pWaterLogicalVolume, false, 0);

  // Air horizontal

  m_pLegHorizontalAir1LogicalVolume = new G4LogicalVolume(
      air_beam_horizontal, Air, "air_leg1horizontalLogical");

  m_pLegHorizontalAir1PhysicalVolume = new G4PVPlacement(
      Rot,
      G4ThreeVector(x_pos_floor_leg1,
                    y_pos_floor_leg1 - x_outer - z_horizontal_outer,
                    z_pos_floor_leg1 + z_outer),
      m_pLegHorizontalAir1LogicalVolume, "Air_support_leg1_horizontal_physical",
      m_pWaterLogicalVolume, false, 0);

  m_pLegHorizontalAir2PhysicalVolume = new G4PVPlacement(
      Rot1,
      G4ThreeVector(x_pos_floor_leg2 - x_outer - z_horizontal_outer,
                    y_pos_floor_leg2, z_pos_floor_leg1 + z_outer),
      m_pLegHorizontalAir1LogicalVolume, "Air_support_leg2_horizontal_physical",
      m_pWaterLogicalVolume, false, 0);

  m_pLegHorizontalAir3PhysicalVolume = new G4PVPlacement(
      Rot,
      G4ThreeVector(x_pos_floor_leg3,
                    y_pos_floor_leg3 - x_outer - z_horizontal_outer,
                    z_pos_floor_leg1 + z_outer),
      m_pLegHorizontalAir1LogicalVolume, "Air_support_leg3_horizontal_physical",
      m_pWaterLogicalVolume, false, 0);

  m_pLegHorizontalAir4PhysicalVolume = new G4PVPlacement(
      Rot1,
      G4ThreeVector(x_pos_floor_leg1 - x_outer - z_horizontal_outer,
                    y_pos_floor_leg1, z_pos_floor_leg1 + z_outer),
      m_pLegHorizontalAir1LogicalVolume, "Air_support_leg4_horizontal_physical",
      m_pWaterLogicalVolume, false, 0);

  m_pLegHorizontalAir5PhysicalVolume = new G4PVPlacement(
      Rot,
      G4ThreeVector(x_pos_floor_leg1,
                    y_pos_floor_leg1 - x_outer - z_horizontal_outer,
                    z_pos_floor_leg1 + z_outer + z_medium_outer * 2.),
      m_pLegHorizontalAir1LogicalVolume, "Air_support_leg5_horizontal_physical",
      m_pWaterLogicalVolume, false, 0);

  m_pLegHorizontalAir6PhysicalVolume = new G4PVPlacement(
      Rot1,
      G4ThreeVector(x_pos_floor_leg2 - x_outer - z_horizontal_outer,
                    y_pos_floor_leg2,
                    z_pos_floor_leg1 + z_outer + z_medium_outer * 2.),
      m_pLegHorizontalAir1LogicalVolume, "Air_support_leg6_horizontal_physical",
      m_pWaterLogicalVolume, false, 0);

  m_pLegHorizontalAir7PhysicalVolume = new G4PVPlacement(
      Rot,
      G4ThreeVector(x_pos_floor_leg3,
                    y_pos_floor_leg3 - x_outer - z_horizontal_outer,
                    z_pos_floor_leg1 + z_outer + z_medium_outer * 2.),
      m_pLegHorizontalAir1LogicalVolume, "Air_support_leg7_horizontal_physical",
      m_pWaterLogicalVolume, false, 0);

  m_pLegHorizontalAir8PhysicalVolume = new G4PVPlacement(
      Rot1,
      G4ThreeVector(x_pos_floor_leg1 - x_outer - z_horizontal_outer,
                    y_pos_floor_leg1,
                    z_pos_floor_leg1 + z_outer + z_medium_outer * 2.),
      m_pLegHorizontalAir1LogicalVolume, "Air_support_leg8_horizontal_physical",
      m_pWaterLogicalVolume, false, 0);

  // Placement of floor 1 beam

  m_pLegPlatformLogicalVolume = new G4LogicalVolume(
      pLegPlatformVolume, SS304LSteel, "leg1platformLogical");
  m_pLegPlatformAirLogicalVolume =
      new G4LogicalVolume(air_beam_platform, Air, "leg1platformLogical");

  m_pLegHorizontalPlatformPhysicalVolume = new G4PVPlacement(
      Rot,
      G4ThreeVector(x_pos_floor_leg1 - x_platform,
                    y_pos_floor_leg1 - x_outer - z_horizontal_outer,
                    z_pos_floor_leg1 + z_outer),
      m_pLegHorizontal1LogicalVolume,
      "plat_support_platform_horizontal_physical", m_pWaterLogicalVolume,
      false, 0);

  m_pLegHorizontalPlatformAirPhysicalVolume = new G4PVPlacement(
      Rot,
      G4ThreeVector(x_pos_floor_leg1 - x_platform,
                    y_pos_floor_leg1 - x_outer - z_horizontal_outer,
                    z_pos_floor_leg1 + z_outer),
      m_pLegHorizontalAir1LogicalVolume,
      "Air_support_leg_platform_horizontal_physical", m_pWaterLogicalVolume,
      false, 0);

  m_pLegHorizontalPlatform1PhysicalVolume = new G4PVPlacement(
      Rot,
      G4ThreeVector(x_pos_floor_leg1 - x_platform1,
                    y_pos_floor_leg1 - x_outer - z_horizontal_outer,
                    z_pos_floor_leg1 + z_outer),
      m_pLegHorizontal1LogicalVolume,
      "plat_support_platform1_horizontal_physical", m_pWaterLogicalVolume,
      false, 0);

  m_pLegHorizontalPlatformAir1PhysicalVolume = new G4PVPlacement(
      Rot,
      G4ThreeVector(x_pos_floor_leg1 - x_platform1,
                    y_pos_floor_leg1 - x_outer - z_horizontal_outer,
                    z_pos_floor_leg1 + z_outer),
      m_pLegHorizontalAir1LogicalVolume,
      "Air_support_leg1_platform_horizontal_physical", m_pWaterLogicalVolume,
      false, 0);

  m_pLegPlatformSmallPhysicalVolume = new G4PVPlacement(
      Rot1,
      G4ThreeVector(0, y_pos_floor_leg2 + x_platform,
                    z_pos_floor_leg1 + z_outer),
      m_pLegPlatformLogicalVolume, "plat_support_platform_small_leg1",
      m_pWaterLogicalVolume, false, 0);

  m_pLegPlatformSmallAirPhysicalVolume = new G4PVPlacement(
      Rot1,
      G4ThreeVector(0, y_pos_floor_leg2 + x_platform,
                    z_pos_floor_leg1 + z_outer),
      m_pLegPlatformAirLogicalVolume, "Air_support_platform_small_leg1",
      m_pWaterLogicalVolume, false, 0);

  m_pLegPlatformSmall1PhysicalVolume = new G4PVPlacement(
      Rot1,
      G4ThreeVector(0, y_pos_floor_leg2 + x_platform1,
                    z_pos_floor_leg1 + z_outer),
      m_pLegPlatformLogicalVolume, "plat_support_platform_small_leg2",
      m_pWaterLogicalVolume, false, 0);

  m_pLegPlatformSmallAir1PhysicalVolume = new G4PVPlacement(
      Rot1,
      G4ThreeVector(0, y_pos_floor_leg2 + x_platform1,
                    z_pos_floor_leg1 + z_outer),
      m_pLegPlatformAirLogicalVolume, "Air_support_platform_small_leg2",
      m_pWaterLogicalVolume, false, 0);

  // Spreader
  G4RotationMatrix *RotSpreader = new G4RotationMatrix;
  RotSpreader->rotateZ(120. * deg);
  RotSpreader->rotateX(90. * deg);
  // RotSpreader->rotateY(120.*deg);

  G4RotationMatrix *RotSpreader1 = new G4RotationMatrix;
  RotSpreader1->rotateZ(240. * deg);
  RotSpreader1->rotateX(90. * deg);

  m_pLegSpreaderPlatLogicalVolume =
      new G4LogicalVolume(pand, SS304LSteel, "leg1Logical");
  m_pLegSpreaderLogicalVolume =
      new G4LogicalVolume(pLegSpreader, SS304LSteel, "leg1Logical");
  m_pLegSpreaderAirLogicalVolume =
      new G4LogicalVolume(air_beam_spreader, Air, "leg1Logical");
  m_pLegSpreader1PhysicalVolume =
      new G4PVPlacement(Rot, G4ThreeVector(0, +z_spreader, z_pos_spreader),
                        m_pLegSpreaderLogicalVolume, "spreader_1_physical",
                        m_pWaterLogicalVolume, false, 0);
  m_pLegSpreader2PhysicalVolume = new G4PVPlacement(
      RotSpreader,
      G4ThreeVector(+z_spreader * cos(30. * deg) + x_spreader * cos(30. * deg),
                    -z_spreader * sin(30. * deg) - x_spreader * sin(30. * deg),
                    z_pos_spreader),
      m_pLegSpreaderLogicalVolume, "spreader_2_physical",
      m_pWaterLogicalVolume, false, 0);
  m_pLegSpreader3PhysicalVolume = new G4PVPlacement(
      RotSpreader1,
      G4ThreeVector(-z_spreader * cos(30. * deg) - x_spreader * cos(30. * deg),
                    -z_spreader * sin(30. * deg) - x_spreader * sin(30. * deg),
                    z_pos_spreader),
      m_pLegSpreaderLogicalVolume, "spreader_3_physical",
      m_pWaterLogicalVolume, false, 0);
  m_pLegSpreader1AirPhysicalVolume = new G4PVPlacement(
      Rot, G4ThreeVector(0, +z_spreader, z_pos_spreader),
      m_pLegSpreaderAirLogicalVolume, "Air_support_leg1_spreader_physical",
      m_pWaterLogicalVolume, false, 0);
  m_pLegSpreader2AirPhysicalVolume = new G4PVPlacement(
      RotSpreader,
      G4ThreeVector(+z_spreader * cos(30. * deg) + x_spreader * cos(30. * deg),
                    -z_spreader * sin(30. * deg) - x_spreader * sin(30. * deg),
                    z_pos_spreader),
      m_pLegSpreaderAirLogicalVolume, "Air_support_leg2_spreader_physical",
      m_pWaterLogicalVolume, false, 0);
  m_pLegSpreader3AirPhysicalVolume = new G4PVPlacement(
      RotSpreader1,
      G4ThreeVector(-z_spreader * cos(30. * deg) - x_spreader * cos(30. * deg),
                    -z_spreader * sin(30. * deg) - x_spreader * sin(30. * deg),
                    z_pos_spreader),
      m_pLegSpreaderAirLogicalVolume, "Air_support_leg3_spreader_physical",
      m_pWaterLogicalVolume, false, 0);
  m_pLegSpreaderPlat1PhysicalVolume = new G4PVPlacement(
      Rot, G4ThreeVector(0., 0., z_pos_spreader + y_spreader + 5.2),
      m_pLegSpreaderPlatLogicalVolume, "spreader_plat_1_physical",
      m_pWaterLogicalVolume, false, 0);
  m_pLegSpreaderPlat2PhysicalVolume = new G4PVPlacement(
      Rot, G4ThreeVector(0., 0., z_pos_spreader - y_spreader - 5.2),
      m_pLegSpreaderPlatLogicalVolume, "spreader_plat_2_physical",
      m_pWaterLogicalVolume, false, 0);
  //+5 is coming from the +10. in the trapdefinition

  // Brace Rods

  G4double z_calc = 3090. * mm;  // from Cad
  G4double y_calc = z_horizontal_outer * 2.;
  G4double tilt_angle_brace =
      90. * deg - 180. * deg * atan(z_calc / y_calc) /
                      pi_g;  /// 54.61211*deg, from calculation.
  G4double R_brace_rods = GetGeometryParameter("R_brace_rods");
  G4double cylinder_height_brace =
      sqrt(z_calc * z_calc + y_calc * y_calc) / 2. -
      (R_brace_rods / cos(tilt_angle_brace)) * sin(tilt_angle_brace) -
      0.1 * mm;  // 2667.89242*mm

  G4Tubs *pcylinderbracerods =
      new G4Tubs("Cylinder_brace_rods", 0., R_brace_rods, cylinder_height_brace,
                 0. * deg, 360. * deg);

  m_pBraceRodLogicalVolume =
      new G4LogicalVolume(pcylinderbracerods, SS304LSteel, "Cylinder_tie_rods");

  G4RotationMatrix *RotBrace = new G4RotationMatrix;
  RotBrace->rotateY(tilt_angle_brace);

  G4double zbrace = z_pos_floor_leg2 - 37.5 * mm;  // offset in the z placement
  m_pBraceRodLow1PhysicalVolume = new G4PVPlacement(
      RotBrace, G4ThreeVector(0., 2250., zbrace), m_pBraceRodLogicalVolume,
      "Brace_Rod_1_support", m_pWaterLogicalVolume, false,
      0);  // 2250 I choose a y plane

  G4RotationMatrix *RotBrace2 = new G4RotationMatrix;
  RotBrace2->rotateX(tilt_angle_brace);

  G4RotationMatrix *RotBrace1 = new G4RotationMatrix;
  RotBrace1->rotateY(90. * deg + (90. * deg - tilt_angle_brace));

  G4RotationMatrix *RotBrace3 = new G4RotationMatrix;
  RotBrace3->rotateX(90. * deg + (90. * deg - tilt_angle_brace));

  m_pBraceRodLow2PhysicalVolume = new G4PVPlacement(
      RotBrace1, G4ThreeVector(0., 2200., zbrace), m_pBraceRodLogicalVolume,
      "Brace_Rod_2_support", m_pWaterLogicalVolume, false, 0);

  m_pBraceRodLow3PhysicalVolume = new G4PVPlacement(
      RotBrace, G4ThreeVector(0., -2250., zbrace), m_pBraceRodLogicalVolume,
      "Brace_Rod_3_support", m_pWaterLogicalVolume, false, 0);

  m_pBraceRodLow4PhysicalVolume = new G4PVPlacement(
      RotBrace1, G4ThreeVector(0., -2200., zbrace), m_pBraceRodLogicalVolume,
      "Brace_Rod_4_support", m_pWaterLogicalVolume, false, 0);

  m_pBraceRodLow5PhysicalVolume = new G4PVPlacement(
      RotBrace2, G4ThreeVector(2250, 0., zbrace), m_pBraceRodLogicalVolume,
      "Brace_Rod_5_support", m_pWaterLogicalVolume, false, 0);

  m_pBraceRodLow6PhysicalVolume = new G4PVPlacement(
      RotBrace3, G4ThreeVector(2200, 0., zbrace), m_pBraceRodLogicalVolume,
      "Brace_Rod_6_support", m_pWaterLogicalVolume, false, 0);

  m_pBraceRodLow7PhysicalVolume = new G4PVPlacement(
      RotBrace2, G4ThreeVector(-2250, 0., zbrace), m_pBraceRodLogicalVolume,
      "Brace_Rod_7_support", m_pWaterLogicalVolume, false, 0);

  m_pBraceRodLow8PhysicalVolume = new G4PVPlacement(
      RotBrace3, G4ThreeVector(-2200, 0., zbrace), m_pBraceRodLogicalVolume,
      "Brace_Rod_8_support", m_pWaterLogicalVolume, false, 0);

  G4double z_calc_2 = 2915. * mm;

  G4double tilt_angle_brace_medium =
      90. * deg - 180. * deg * atan(z_calc_2 / y_calc) / pi_g;  // 56.173369

  G4double cylinder_height_brace_medium =
      sqrt(z_calc_2 * z_calc_2 + y_calc * y_calc) / 2. -
      (R_brace_rods / cos(tilt_angle_brace_medium)) *
          sin(tilt_angle_brace_medium) -
      0.1 * mm;  // 2616.801482*mm
  G4Tubs *pcylinderbracerods_medium =
      new G4Tubs("Cylinder_brace_rods", 0., R_brace_rods,
                 cylinder_height_brace_medium, 0. * deg, 360. * deg);
  m_pBraceRodMediumLogicalVolume = new G4LogicalVolume(
      pcylinderbracerods_medium, SS304LSteel, "Cylinder_tie_rods");

  G4double zbrace_medium = z_pos_floor_leg2 + z_outer + z_medium_outer;

  G4RotationMatrix *RotBracemedium = new G4RotationMatrix;
  RotBracemedium->rotateY(tilt_angle_brace_medium);

  G4RotationMatrix *RotBracemedium1 = new G4RotationMatrix;
  RotBracemedium1->rotateY(90. * deg + (90. * deg - tilt_angle_brace_medium));

  G4RotationMatrix *RotBracemedium2 = new G4RotationMatrix;
  RotBracemedium2->rotateX(tilt_angle_brace_medium);

  G4RotationMatrix *RotBracemedium3 = new G4RotationMatrix;
  RotBracemedium3->rotateX(90. * deg + (90. * deg - tilt_angle_brace_medium));

  m_pBraceRodMedium1PhysicalVolume = new G4PVPlacement(
      RotBracemedium, G4ThreeVector(0., 2250., zbrace_medium),
      m_pBraceRodMediumLogicalVolume, "Brace_Rod_Medium_1_support",
      m_pWaterLogicalVolume, false, 0);

  m_pBraceRodMedium2PhysicalVolume = new G4PVPlacement(
      RotBracemedium1, G4ThreeVector(0., 2200., zbrace_medium),
      m_pBraceRodMediumLogicalVolume, "Brace_Rod_Medium_2_support",
      m_pWaterLogicalVolume, false, 0);

  m_pBraceRodMedium3PhysicalVolume = new G4PVPlacement(
      RotBracemedium, G4ThreeVector(0., -2250., zbrace_medium),
      m_pBraceRodMediumLogicalVolume, "Brace_Rod_Medium_3_support",
      m_pWaterLogicalVolume, false, 0);

  m_pBraceRodMedium4PhysicalVolume = new G4PVPlacement(
      RotBracemedium1, G4ThreeVector(0., -2200., zbrace_medium),
      m_pBraceRodMediumLogicalVolume, "Brace_Rod_Medium_4_support",
      m_pWaterLogicalVolume, false, 0);

  m_pBraceRodMedium5PhysicalVolume = new G4PVPlacement(
      RotBracemedium2, G4ThreeVector(2250., 0., zbrace_medium),
      m_pBraceRodMediumLogicalVolume, "Brace_Rod_Medium_5_support",
      m_pWaterLogicalVolume, false, 0);

  m_pBraceRodMedium6PhysicalVolume = new G4PVPlacement(
      RotBracemedium3, G4ThreeVector(2200., 0., zbrace_medium),
      m_pBraceRodMediumLogicalVolume, "Brace_Rod_Medium_6_support",
      m_pWaterLogicalVolume, false, 0);

  m_pBraceRodMedium7PhysicalVolume = new G4PVPlacement(
      RotBracemedium2, G4ThreeVector(-2250., 0., zbrace_medium),
      m_pBraceRodMediumLogicalVolume, "Brace_Rod_Medium_7_support",
      m_pWaterLogicalVolume, false, 0);

  m_pBraceRodMedium8PhysicalVolume = new G4PVPlacement(
      RotBracemedium3, G4ThreeVector(-2200., 0., zbrace_medium),
      m_pBraceRodMediumLogicalVolume, "Brace_Rod_Medium_8_support",
      m_pWaterLogicalVolume, false, 0);

  // Tie Rods

  G4double R_tie_rods = GetGeometryParameter("R_tie_rods");

  G4double y_tie_rod1 = z_spreader * 2.;  //

  G4double y_tie_rod2 =
      -(z_spreader * 2. * sin(30. * deg) + x_spreader * sin(30. * deg) -
        50. * sin(30. * deg));  // 220.
  G4double x_tie_rod2 = z_spreader * 2. * cos(30. * deg) +
                        x_spreader * cos(30. * deg) * cos(30. * deg) -
                        50. * cos(30. * deg);  //-50 position tie rod
  G4double y_2_tie_rod2 =
      -(z_spreader * 2. * sin(30. * deg) + x_spreader * sin(30. * deg) -
        225. * sin(30. * deg));  // 225 position tie rod
  G4double x_2_tie_rod2 = z_spreader * 2. * cos(30. * deg) +
                          x_spreader * cos(30. * deg) * cos(30. * deg) -
                          225. * cos(30. * deg);  //
  G4double cylinder_height_tie = 929. * mm;       // from geantino simulation
  G4double cylinder_height_tie_2 = 583. * mm;     // from geantino
  G4double z_tie_rod =
      827. + cryo_offset + cylinder_height_tie;  // from geantino simulation
  G4double z_2_tie_rod = 827. + cryo_offset + 2. * 930. + y_spreader * 2. +
                         cylinder_height_tie_2;  // geantino sim

  G4Tubs *pcylindertierods =
      new G4Tubs("Cylinder_tie_rods", 0., R_tie_rods, cylinder_height_tie,
                 0. * deg, 360. * deg);
  G4Tubs *pcylindertierods2 =
      new G4Tubs("Cylinder_tie_rods", 0., R_tie_rods, cylinder_height_tie_2,
                 0. * deg, 360. * deg);

  m_pTieRodLogicalVolume =
      new G4LogicalVolume(pcylindertierods, SS304LSteel, "Cylinder_tie_rods");
  m_pTie2RodLogicalVolume =
      new G4LogicalVolume(pcylindertierods2, SS304LSteel, "Cylinder_tie_rods");

  if (pNTversion != "XENONnT") {
    // MS180213 to avoid overlap between the WaterDisplacer and
    // the three rods connecting the SupportStructure and the
    // Cryostat
    m_pTieRod1PhysicalVolume = new G4PVPlacement(
        0, G4ThreeVector(0., y_tie_rod1 - 30., z_tie_rod),
        m_pTieRodLogicalVolume, "Tie_Rod_1_support", m_pWaterLogicalVolume,
        false, 0);  //-30 position tie rod
    m_pTieRod2PhysicalVolume =
    new G4PVPlacement(0, G4ThreeVector(x_tie_rod2, y_tie_rod2, z_tie_rod),
        m_pTieRodLogicalVolume, "Tie_Rod_2_support",
        m_pWaterLogicalVolume, false, 0);
    m_pTieRod3PhysicalVolume =
    new G4PVPlacement(0, G4ThreeVector(-x_tie_rod2, y_tie_rod2, z_tie_rod),
        m_pTieRodLogicalVolume, "Tie_Rod_3_support",
        m_pWaterLogicalVolume, false, 0);
  }

  m_pTie2Rod1PhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(0., y_tie_rod1 - 200., z_2_tie_rod),
      m_pTie2RodLogicalVolume, "Tie_Rod_2half_1_support",
      m_pWaterLogicalVolume, false, 0);  //-200 position tie rod
  m_pTie2Rod2PhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(x_2_tie_rod2, y_2_tie_rod2, z_2_tie_rod),
      m_pTie2RodLogicalVolume, "Tie_Rod_2half_2_support",
      m_pWaterLogicalVolume, false, 0);
  m_pTie2Rod3PhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(-x_2_tie_rod2, y_2_tie_rod2, z_2_tie_rod),
      m_pTie2RodLogicalVolume, "Tie_Rod_2half_3_support",
      m_pWaterLogicalVolume, false, 0);

  // From now on, the mother volume is the water cone (m_pWaterConsLogicalVolume)

  // position of the tilt legs in the cons

  G4double x_cons_leg1 = x_pos_floor_leg1 + 2. * x_center_tilt_beam -
                         z_tilt_cons_beam * sintilt * cosrot -
                         2. * (x_outer / costilt) * sintilt * sintilt * cosrot;
  G4double y_cons_leg1 = y_pos_floor_leg1 + 2. * y_center_tilt_beam -
                         z_tilt_cons_beam * sintilt * cosrot -
                         2. * (x_outer / costilt) * sintilt * sintilt * cosrot;
  G4double z_cons_leg1 =
      -(dWaterConsZed / 2. - z_tilt_cons_beam * costilt) + x_outer * sintilt;

  // placement of the tilt legs in the cons

  m_pLegTiltedCons1LogicalVolume =
      new G4LogicalVolume(pLegTiltedConsVolume, SS304LSteel, "legtilted1cons");
  m_pLegTiltedCons1PhysicalVolume = new G4PVPlacement(
      Rot6, G4ThreeVector(x_cons_leg1, y_cons_leg1, z_cons_leg1),
      m_pLegTiltedCons1LogicalVolume, "support_legcons_tilted_physical_1",
      m_pWaterConsLogicalVolume, false, 0);
  m_pLegTiltedCons2LogicalVolume =
      new G4LogicalVolume(pLegTiltedCons1Volume, SS304LSteel, "legtilted2cons");
  m_pLegTiltedCons2PhysicalVolume = new G4PVPlacement(
      Rot4, G4ThreeVector(-x_cons_leg1, y_cons_leg1, z_cons_leg1),
      m_pLegTiltedCons2LogicalVolume, "support_legcons_tilted_physical_2",
      m_pWaterConsLogicalVolume, false, 0);
  m_pLegTiltedCons3LogicalVolume =
      new G4LogicalVolume(pLegTiltedConsVolume, SS304LSteel, "legtilted3cons");
  m_pLegTiltedCons3PhysicalVolume = new G4PVPlacement(
      Rot3, G4ThreeVector(x_cons_leg1, -y_cons_leg1, z_cons_leg1),
      m_pLegTiltedCons3LogicalVolume, "support_legcons_tilted_physical_3",
      m_pWaterConsLogicalVolume, false, 0);
  m_pLegTiltedCons4LogicalVolume =
      new G4LogicalVolume(pLegTiltedCons1Volume, SS304LSteel, "legtilted4cons");
  m_pLegTiltedCons4PhysicalVolume = new G4PVPlacement(
      Rot5, G4ThreeVector(-x_cons_leg1, -y_cons_leg1, z_cons_leg1),
      m_pLegTiltedCons4LogicalVolume, "support_legcons_tilted_physical_4",
      m_pWaterConsLogicalVolume, false, 0);

  // Air
  m_pLegTiltedConsAir1LogicalVolume =
      new G4LogicalVolume(air_beam_tilted_cons, Air, "air_legtilted1cons");
  m_pLegTiltedConsAir1PhysicalVolume = new G4PVPlacement(
      Rot6, G4ThreeVector(x_cons_leg1, y_cons_leg1, z_cons_leg1),
      m_pLegTiltedConsAir1LogicalVolume,
      "Air_support_legcons_tilted_physical_1", m_pWaterConsLogicalVolume, false,
      0);

  m_pLegTiltedConsAir2PhysicalVolume = new G4PVPlacement(
      Rot4, G4ThreeVector(-x_cons_leg1, y_cons_leg1, z_cons_leg1),
      m_pLegTiltedConsAir1LogicalVolume,
      "Air_support_legcons_tilted_physical_2", m_pWaterConsLogicalVolume, false,
      0);

  m_pLegTiltedConsAir3PhysicalVolume = new G4PVPlacement(
      Rot3, G4ThreeVector(x_cons_leg1, -y_cons_leg1, z_cons_leg1),
      m_pLegTiltedConsAir1LogicalVolume,
      "Air_support_legcons_tilted_physical_3", m_pWaterConsLogicalVolume, false,
      0);

  m_pLegTiltedConsAir4PhysicalVolume = new G4PVPlacement(
      Rot5, G4ThreeVector(-x_cons_leg1, -y_cons_leg1, z_cons_leg1),
      m_pLegTiltedConsAir1LogicalVolume,
      "Air_support_legcons_tilted_physical_4", m_pWaterConsLogicalVolume, false,
      0);

  // position of the top legs
  G4double x_center_top_beam1 =
      x_cons_leg1 - z_tilt_cons_beam * sintilt * cosrot -
      2. * (x_outer / costilt) * sintilt * sintilt * cosrot;

  G4double y_center_top_beam1 =
      y_cons_leg1 - z_tilt_cons_beam * sintilt * cosrot -
      2. * (x_outer / costilt) * sintilt * sintilt * cosrot;

  // lenght tilt beam on the top
  G4double z_tilt_beam_top =
      sqrt(4. * x_center_top_beam1 * x_center_top_beam1 +
           4. * y_center_top_beam1 * y_center_top_beam1) *
      0.5;
  G4double z_tilt_beam_top_2 = z_tilt_beam_top * 0.5 - x_outer * 0.5;

  // Top Tilted Volume
  G4UnionSolid *pLegTop1Volume = ConstructBeam(
      x_outer, y_outer, z_tilt_beam_top, x_inner, y_inner, z_tilt_beam_top);
  G4UnionSolid *pLegTop2Volume = ConstructBeam(
      x_outer, y_outer, z_tilt_beam_top_2, x_inner, y_inner, z_tilt_beam_top_2);
  G4Box *air_beam_top_1 = new G4Box("box_top", x_air, y_air, z_tilt_beam_top);
  G4Box *air_beam_top_2 =
      new G4Box("box_top_2", x_air, y_air, z_tilt_beam_top_2);

  // zed position of the top legs

  G4double z_center_top_beam1 =
      z_cons_leg1 + z_tilt_cons_beam * costilt + x_outer * sintilt + x_outer;

  G4RotationMatrix *Rot7 = new G4RotationMatrix;
  Rot7->rotateZ(-rotation_angle);
  Rot7->rotateY(90. * deg);
  G4RotationMatrix *Rot8 = new G4RotationMatrix;
  Rot8->rotateZ(rotation_angle);
  Rot8->rotateY(90. * deg);

  // placement of the top leg
  m_pLegTopLogicalVolume1 =
      new G4LogicalVolume(pLegTop1Volume, SS304LSteel, "legtop1");
  m_pLegTopPhysicalVolume1 = new G4PVPlacement(
      Rot7,
      G4ThreeVector(x_center_top_beam1 - z_tilt_beam_top * cosrot,
                    y_center_top_beam1 - z_tilt_beam_top * cosrot,
                    z_center_top_beam1),
      m_pLegTopLogicalVolume1, "support_leg1_top", m_pWaterConsLogicalVolume,
      false, 0);

  m_pLegTopLogicalVolume2 =
      new G4LogicalVolume(pLegTop2Volume, SS304LSteel, "legtop1");
  m_pLegTopPhysicalVolume2 = new G4PVPlacement(
      Rot8,
      G4ThreeVector(-x_center_top_beam1 + z_tilt_beam_top_2 * cosrot,
                    y_center_top_beam1 - z_tilt_beam_top_2 * cosrot,
                    z_center_top_beam1),
      m_pLegTopLogicalVolume2, "support_leg2_top", m_pWaterConsLogicalVolume,
      false, 0);

  m_pLegTopPhysicalVolume3 = new G4PVPlacement(
      Rot8,
      G4ThreeVector(x_center_top_beam1 - z_tilt_beam_top_2 * cosrot,
                    -y_center_top_beam1 + z_tilt_beam_top_2 * cosrot,
                    z_center_top_beam1),
      m_pLegTopLogicalVolume2, "support_leg3_top", m_pWaterConsLogicalVolume,
      false, 0);

  // air

  m_pLegTopAirLogicalVolume1 =
      new G4LogicalVolume(air_beam_top_1, Air, "air_legtop1");
  m_pLegTopAirPhysicalVolume1 = new G4PVPlacement(
      Rot7,
      G4ThreeVector(x_center_top_beam1 - z_tilt_beam_top * cosrot,
                    y_center_top_beam1 - z_tilt_beam_top * cosrot,
                    z_center_top_beam1),
      m_pLegTopAirLogicalVolume1, "Air_support_leg1_top",
      m_pWaterConsLogicalVolume, false, 0);
  m_pLegTopAirLogicalVolume2 =
      new G4LogicalVolume(air_beam_top_2, Air, "air_legtop1");
  m_pLegTopAirPhysicalVolume2 = new G4PVPlacement(
      Rot8,
      G4ThreeVector(-x_center_top_beam1 + z_tilt_beam_top_2 * cosrot,
                    y_center_top_beam1 - z_tilt_beam_top_2 * cosrot,
                    z_center_top_beam1),
      m_pLegTopAirLogicalVolume2, "Air_support_leg2_top",
      m_pWaterConsLogicalVolume, false, 0);
  m_pLegTopAirPhysicalVolume3 = new G4PVPlacement(
      Rot8,
      G4ThreeVector(x_center_top_beam1 - z_tilt_beam_top_2 * cosrot,
                    -y_center_top_beam1 + z_tilt_beam_top_2 * cosrot,
                    z_center_top_beam1),
      m_pLegTopAirLogicalVolume2, "Air_support_leg3_top",
      m_pWaterConsLogicalVolume, false, 0);

  G4double position = 0.5 * 1237. * mm + x_outer;  // from cad
  // zed of square top legs
  G4double z_top_vertical_leg = position - x_outer - x_outer * sqrt(2);

  G4double position1 = 0.5 * 1449. * mm + x_outer;  // from cad
  // zed of square top legs
  G4double z_top_vertical_leg_1 = position1 - x_outer - x_outer * sqrt(2);

  G4UnionSolid *pLegTopVerticalVolume1 =
      ConstructBeam(x_outer, y_outer, z_top_vertical_leg, x_inner, y_inner,
                    z_top_vertical_leg);
  G4UnionSolid *pLegTopVerticalVolume2 =
      ConstructBeam(x_outer, y_outer, z_top_vertical_leg_1, x_inner, y_inner,
                    z_top_vertical_leg_1);
  G4Box *air_beam_top_3 =
      new G4Box("box_top_3", x_air, y_air, z_top_vertical_leg);
  G4Box *air_beam_top_4 =
      new G4Box("box_top_4", x_air, y_air, z_top_vertical_leg_1);

  m_pLegTopLogicalVolume3 =
      new G4LogicalVolume(pLegTopVerticalVolume1, SS304LSteel, "legtop1");
  m_pLegTopPhysicalVolume4 =
      new G4PVPlacement(Rot1, G4ThreeVector(0, position, z_center_top_beam1),
                        m_pLegTopLogicalVolume3, "support_leg4_top",
                        m_pWaterConsLogicalVolume, false, 0);
  m_pLegTopPhysicalVolume5 =
      new G4PVPlacement(Rot1, G4ThreeVector(0, -position, z_center_top_beam1),
                        m_pLegTopLogicalVolume3, "support_leg5_top",
                        m_pWaterConsLogicalVolume, false, 0);

  m_pLegTopLogicalVolume4 =
      new G4LogicalVolume(pLegTopVerticalVolume2, SS304LSteel, "legtop1");
  m_pLegTopPhysicalVolume6 =
      new G4PVPlacement(Rot, G4ThreeVector(position1, 0., z_center_top_beam1),
                        m_pLegTopLogicalVolume4, "support_leg6_top",
                        m_pWaterConsLogicalVolume, false, 0);
  m_pLegTopPhysicalVolume7 =
      new G4PVPlacement(Rot, G4ThreeVector(-position1, 0., z_center_top_beam1),
                        m_pLegTopLogicalVolume4, "support_leg7_top",
                        m_pWaterConsLogicalVolume, false, 0);

  // Air
  m_pLegTopAirLogicalVolume3 =
      new G4LogicalVolume(air_beam_top_3, Air, "air_legtop1");
  m_pLegTopAirPhysicalVolume4 =
      new G4PVPlacement(Rot1, G4ThreeVector(0, position, z_center_top_beam1),
                        m_pLegTopAirLogicalVolume3, "Air_support_leg4_top",
                        m_pWaterConsLogicalVolume, false, 0);
  m_pLegTopAirPhysicalVolume5 =
      new G4PVPlacement(Rot1, G4ThreeVector(0, -position, z_center_top_beam1),
                        m_pLegTopAirLogicalVolume3, "Air_support_leg5_top",
                        m_pWaterConsLogicalVolume, false, 0);
  m_pLegTopAirLogicalVolume4 =
      new G4LogicalVolume(air_beam_top_4, Air, "legtop1");
  m_pLegTopAirPhysicalVolume6 =
      new G4PVPlacement(Rot, G4ThreeVector(position1, 0., z_center_top_beam1),
                        m_pLegTopAirLogicalVolume4, "Air_support_leg6_top",
                        m_pWaterConsLogicalVolume, false, 0);
  m_pLegTopAirPhysicalVolume7 =
      new G4PVPlacement(Rot, G4ThreeVector(-position1, 0., z_center_top_beam1),
                        m_pLegTopAirLogicalVolume4, "Air_support_leg7_top",
                        m_pWaterConsLogicalVolume, false, 0);

  // Tie Rods

  G4double cylinder_height_tie_cons = 508. * mm;

  G4Tubs *pcylindertierodscons =
      new G4Tubs("Cylinder_tie_rods", 0., R_tie_rods, cylinder_height_tie_cons,
                 0. * deg, 360. * deg);

  m_pTieConsRodLogicalVolume = new G4LogicalVolume(
      pcylindertierodscons, SS304LSteel, "Cylinder_tie_rods");

  G4double z_tie_rod_cons = -(dWaterConsZed / 2. - cylinder_height_tie_cons);

  m_pTieRodCons1PhysicalVolume =
      new G4PVPlacement(0, G4ThreeVector(0., y_tie_rod1 - 200., z_tie_rod_cons),
                        m_pTieConsRodLogicalVolume, "Tie_Rod_Cons_1_support",
                        m_pWaterConsLogicalVolume, false, 0);
  m_pTieRodCons2PhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(x_2_tie_rod2, y_2_tie_rod2, z_tie_rod_cons),
      m_pTieConsRodLogicalVolume, "Tie_Rod_Cons_2_support",
      m_pWaterConsLogicalVolume, false, 0);
  m_pTieRodCons3PhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(-x_2_tie_rod2, y_2_tie_rod2, z_tie_rod_cons),
      m_pTieConsRodLogicalVolume, "Tie_Rod_Cons_3_support",
      m_pWaterConsLogicalVolume, false, 0);

  // Reflector for support structure //
  G4OpticalSurface* OpSurface = new G4OpticalSurface("OpSurface");
  OpSurface->SetType(dielectric_dielectric);
  OpSurface->SetModel(unified);
  OpSurface->SetFinish(groundfrontpainted);

  const G4int NUM = 2;
  G4double pp[NUM] = {1.*eV, 5.*eV};
  G4double reflectivity_W[NUM] = {0.99, 0.99};

  G4MaterialPropertiesTable* SMPT_W = new G4MaterialPropertiesTable();
  SMPT_W->AddProperty("REFLECTIVITY",pp,reflectivity_W,NUM);
  OpSurface->SetMaterialPropertiesTable(SMPT_W);

  new G4LogicalBorderSurface("m_pLegFloor1PhysicalVolume_Surface",
      m_pWaterPhysicalVolume, m_pLegFloor1PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pLegConnection1PhysicalVolume_Surface",
      m_pWaterPhysicalVolume, m_pLegConnection1PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pLegConnection2PhysicalVolume_Surface",
      m_pWaterPhysicalVolume, m_pLegConnection2PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pLegConnection3PhysicalVolume_Surface",
      m_pWaterPhysicalVolume, m_pLegConnection3PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pLegConnection4PhysicalVolume_Surface",
      m_pWaterPhysicalVolume, m_pLegConnection4PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pLegFloor2PhysicalVolume_Surface",
      m_pWaterPhysicalVolume, m_pLegFloor2PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pLegFloor3PhysicalVolume_Surface",
      m_pWaterPhysicalVolume, m_pLegFloor3PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pLegFloor4PhysicalVolume_Surface",
      m_pWaterPhysicalVolume, m_pLegFloor4PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pLegTilted1PhysicalVolume_Surface",
      m_pWaterPhysicalVolume, m_pLegTilted1PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pLegTilted2PhysicalVolume_Surface",
      m_pWaterPhysicalVolume, m_pLegTilted2PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pLegTilted3PhysicalVolume_Surface",
      m_pWaterPhysicalVolume, m_pLegTilted3PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pLegTilted4PhysicalVolume_Surface",
      m_pWaterPhysicalVolume, m_pLegTilted4PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pLegMedium1PhysicalVolume_Surface",
      m_pWaterPhysicalVolume, m_pLegMedium1PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pLegMedium2PhysicalVolume_Surface",
      m_pWaterPhysicalVolume, m_pLegMedium2PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pLegMedium3PhysicalVolume_Surface",
      m_pWaterPhysicalVolume, m_pLegMedium3PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pLegMedium4PhysicalVolume_Surface",
      m_pWaterPhysicalVolume, m_pLegMedium4PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pLegHorizontal1PhysicalVolume_Surface",
      m_pWaterPhysicalVolume, m_pLegHorizontal1PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pLegHorizontal2PhysicalVolume_Surface",
      m_pWaterPhysicalVolume, m_pLegHorizontal2PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pLegHorizontal3PhysicalVolume_Surface",
      m_pWaterPhysicalVolume, m_pLegHorizontal3PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pLegHorizontal4PhysicalVolume_Surface",
      m_pWaterPhysicalVolume, m_pLegHorizontal4PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pLegHorizontal5PhysicalVolume_Surface",
      m_pWaterPhysicalVolume, m_pLegHorizontal5PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pLegHorizontal6PhysicalVolume_Surface",
      m_pWaterPhysicalVolume, m_pLegHorizontal6PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pLegHorizontal7PhysicalVolume_Surface",
      m_pWaterPhysicalVolume, m_pLegHorizontal7PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pLegHorizontal8PhysicalVolume_Surface",
      m_pWaterPhysicalVolume, m_pLegHorizontal8PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pLegHorizontalPlatformPhysicalVolume_Surface",
      m_pWaterPhysicalVolume, m_pLegHorizontalPlatformPhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pLegHorizontalPlatform1PhysicalVolume_Surface",
      m_pWaterPhysicalVolume, m_pLegHorizontalPlatform1PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pLegPlatformSmallPhysicalVolume_Surface",
      m_pWaterPhysicalVolume, m_pLegPlatformSmallPhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pLegPlatformSmall1PhysicalVolume_Surface",
      m_pWaterPhysicalVolume, m_pLegPlatformSmall1PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pLegSpreader1PhysicalVolume_Surface",
      m_pWaterPhysicalVolume, m_pLegSpreader1PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pLegSpreader2PhysicalVolume_Surface",
      m_pWaterPhysicalVolume, m_pLegSpreader2PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pLegSpreader3PhysicalVolume_Surface",
      m_pWaterPhysicalVolume, m_pLegSpreader3PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pLegSpreaderPlat1PhysicalVolume_Surface",
      m_pWaterPhysicalVolume, m_pLegSpreaderPlat1PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pLegSpreaderPlat2PhysicalVolume_Surface",
      m_pWaterPhysicalVolume, m_pLegSpreaderPlat2PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pBraceRodLow1PhysicalVolume_Surface",
      m_pWaterPhysicalVolume, m_pBraceRodLow1PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pBraceRodLow2PhysicalVolume_Surface",
      m_pWaterPhysicalVolume, m_pBraceRodLow2PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pBraceRodLow3PhysicalVolume_Surface",
      m_pWaterPhysicalVolume, m_pBraceRodLow3PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pBraceRodLow4PhysicalVolume_Surface",
      m_pWaterPhysicalVolume, m_pBraceRodLow4PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pBraceRodLow5PhysicalVolume_Surface",
      m_pWaterPhysicalVolume, m_pBraceRodLow5PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pBraceRodLow6PhysicalVolume_Surface",
      m_pWaterPhysicalVolume, m_pBraceRodLow6PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pBraceRodLow7PhysicalVolume_Surface",
      m_pWaterPhysicalVolume, m_pBraceRodLow7PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pBraceRodLow8PhysicalVolume_Surface",
      m_pWaterPhysicalVolume, m_pBraceRodLow8PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pBraceRodMedium1PhysicalVolume_Surface",
      m_pWaterPhysicalVolume, m_pBraceRodMedium1PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pBraceRodMedium2PhysicalVolume_Surface",
      m_pWaterPhysicalVolume, m_pBraceRodMedium2PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pBraceRodMedium3PhysicalVolume_Surface",
      m_pWaterPhysicalVolume, m_pBraceRodMedium3PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pBraceRodMedium4PhysicalVolume_Surface",
      m_pWaterPhysicalVolume, m_pBraceRodMedium4PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pBraceRodMedium5PhysicalVolume_Surface",
      m_pWaterPhysicalVolume, m_pBraceRodMedium5PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pBraceRodMedium6PhysicalVolume_Surface",
      m_pWaterPhysicalVolume, m_pBraceRodMedium6PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pBraceRodMedium7PhysicalVolume_Surface",
      m_pWaterPhysicalVolume, m_pBraceRodMedium7PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pBraceRodMedium8PhysicalVolume_Surface",
      m_pWaterPhysicalVolume, m_pBraceRodMedium8PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pTie2Rod1PhysicalVolume_Surface",
      m_pWaterPhysicalVolume, m_pTie2Rod1PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pTie2Rod2PhysicalVolume_Surface",
      m_pWaterPhysicalVolume, m_pTie2Rod2PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pTie2Rod3PhysicalVolume_Surface",
      m_pWaterPhysicalVolume, m_pTie2Rod3PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pLegTiltedCons1PhysicalVolume_Surface",
      m_pWaterConsPhysicalVolume, m_pLegTiltedCons1PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pLegTiltedCons2PhysicalVolume_Surface",
      m_pWaterConsPhysicalVolume, m_pLegTiltedCons2PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pLegTiltedCons3PhysicalVolume_Surface",
      m_pWaterConsPhysicalVolume, m_pLegTiltedCons3PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pLegTiltedCons4PhysicalVolume_Surface",
      m_pWaterConsPhysicalVolume, m_pLegTiltedCons4PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pLegTopPhysicalVolume1_Surface",
      m_pWaterConsPhysicalVolume, m_pLegTopPhysicalVolume1, OpSurface);

  new G4LogicalBorderSurface("m_pLegTopPhysicalVolume2_Surface",
      m_pWaterConsPhysicalVolume, m_pLegTopPhysicalVolume2, OpSurface);

  new G4LogicalBorderSurface("m_pLegTopPhysicalVolume3_Surface",
      m_pWaterConsPhysicalVolume, m_pLegTopPhysicalVolume3, OpSurface);

  new G4LogicalBorderSurface("m_pLegTopPhysicalVolume4_Surface",
      m_pWaterConsPhysicalVolume, m_pLegTopPhysicalVolume4, OpSurface);

  new G4LogicalBorderSurface("m_pLegTopPhysicalVolume5_Surface",
      m_pWaterConsPhysicalVolume, m_pLegTopPhysicalVolume5, OpSurface);

  new G4LogicalBorderSurface("m_pLegTopPhysicalVolume6_Surface",
      m_pWaterConsPhysicalVolume, m_pLegTopPhysicalVolume6, OpSurface);

  new G4LogicalBorderSurface("m_pLegTopPhysicalVolume7_Surface",
      m_pWaterConsPhysicalVolume, m_pLegTopPhysicalVolume7, OpSurface);

  new G4LogicalBorderSurface("m_pTieRodCons1PhysicalVolume_Surface",
      m_pWaterConsPhysicalVolume, m_pTieRodCons1PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pTieRodCons2PhysicalVolume_Surface",
      m_pWaterConsPhysicalVolume, m_pTieRodCons2PhysicalVolume, OpSurface);

  new G4LogicalBorderSurface("m_pTieRodCons3PhysicalVolume_Surface",
      m_pWaterConsPhysicalVolume, m_pTieRodCons3PhysicalVolume, OpSurface);
}

void Xenon1tDetectorConstruction::ConstructLeadBrick() {
  //***Andrew 22/08/12***//
  // Same as Xe100

  // Dimensions
  G4double LeadBrick_len_x = GetGeometryParameter("LeadBrick_len_x");
  G4double LeadBrick_len_y = GetGeometryParameter("LeadBrick_len_y");
  G4double LeadBrick_len_z = GetGeometryParameter("LeadBrick_len_z");

  // Position
  G4double LeadBrick_pos_x = GetGeometryParameter("LeadBrick_pos_x");
  G4double LeadBrick_pos_y = GetGeometryParameter("LeadBrick_pos_y");
  G4double LeadBrick_pos_z = GetGeometryParameter("LeadBrick_pos_z");

  // Have brick length facing detector
  G4Box *LeadBrick = new G4Box("LeadBrick", LeadBrick_len_x / 2,
                               LeadBrick_len_y / 2, LeadBrick_len_z / 2);
  G4Material *Lead = G4Material::GetMaterial("G4_Pb");
  m_pLeadBrickLogicalVolume = new G4LogicalVolume(LeadBrick, Lead, "LeadBrick");
  m_pLeadBrickPhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(LeadBrick_pos_x, LeadBrick_pos_y, LeadBrick_pos_z),
      m_pLeadBrickLogicalVolume, "LeadBrick_phys", m_pWaterLogicalVolume, false,
      0);
}

void Xenon1tDetectorConstruction::ConstructCalibrationSource(
    G4String pCalibrationSource) {
  //***Andrew, Jacques 17/01/13***//
  // Update: Jacques 2016/04/18
  G4double dGlobalOffset_Z = GetGeometryParameter("TankOffsetZ");
  G4double dOuterCryostatOffsetZ = GetGeometryParameter("OuterCryostatOffsetZ");
  G4double dCryo_R = 0.5 * GetGeometryParameter("OuterCryostatOuterDiameter");
  G4double dBeamPipeAzimuth = GetGeometryParameter("BeamPipeAzimuth");
  G4double dBeamPipeDeclination = GetGeometryParameter("BeamPipeDeclination");
  G4double dWT_R = GetGeometryParameter("WaterTankOuterRadius");
  G4double dPipe_halfL = 0.5 * GetGeometryParameter("BeamPipeLength");
  G4double dPipeOffset_Z = GetGeometryParameter("BeamPipeOffset_z");
  G4double dContainer_R = 0.5 * GetGeometryParameter("GeneratorContainer_oD");
  G4double dNGregionOffset_Z = GetGeometryParameter("NGRegionOffset_z");
  G4double dWT_thickness = GetGeometryParameter("WaterTankThickness");

  G4String pIBeltCavity = pIBeltCavityMaterial;
  G4String pTunsgtenPlateHole = pTunsgtenPlateHoleMaterial;

  // Positions
  // Global Positions obtained from macro (assuming that position is given
  // relative to lab co-ordinate frame)
  G4double Source_X = 0;
  G4double Source_Y = 0;
  G4double Source_Z = 0;
  if (pBeamPipeActive == true) {
    G4bool pBuildMaterials = false;
    // Neutron Generator sits outside the WT inside the Lab Logical Volume. For
    // BeamPipe position is fixed.
    if (m_iVerbosityLevel >= 1)
      G4cout << " Beam Pipe Chosen as Calibration Source. Setting Neutron "
                "Generator Position to Default."
             << G4endl;
    Source_X = cos(dBeamPipeAzimuth - 1.15 * deg) * (dWT_R + dContainer_R);
    Source_Y = -sin(dBeamPipeAzimuth - 1.15 * deg) * (dWT_R + dContainer_R);
    Source_Z = dPipeOffset_Z + 2 * dGlobalOffset_Z + dOuterCryostatOffsetZ +
               (dWT_R - dCryo_R - 7.07 * cm -
                dPipe_halfL * cos(dBeamPipeDeclination) + dContainer_R) *
                   tan(dBeamPipeDeclination) +
               dNGregionOffset_Z;
    if (m_iVerbosityLevel >= 1)
      G4cout << " Position set to :" << Source_X << " " << Source_Y << " "
             << Source_Z << " mm" << G4endl;
    m_pCalibrationSource = new Xenon1tCalibrationSource(
        pCalibrationSource, m_pLabLogicalVolume, Source_X, Source_Y, Source_Z,
        pBuildMaterials, "", "");
  } else if (pCalibrationSource == "BeamPipe") {
    // BeamPipe Positioning is fixed
    G4double dExtra = 151.6 * mm;  // to prevent overlap with SS_OuterCryostat
    G4double dZExtra =
        15. * cm;  // to prevent overlap with Brace_Rod_Medium_4_support
    Source_X = cos(dBeamPipeAzimuth) *
               (dCryo_R + dExtra + cos(dBeamPipeDeclination) * dPipe_halfL);
    Source_Y = -sin(dBeamPipeAzimuth) *
               (dCryo_R + dExtra + cos(dBeamPipeDeclination) * dPipe_halfL);
    Source_Z =
        dPipeOffset_Z + dOuterCryostatOffsetZ - dGlobalOffset_Z - dZExtra;
    // G4cout<<" Position set to :"<<Source_X<<" "<<Source_Y<<" "<<Source_Z<<"
    // mm"<<G4endl;
    m_pCalibrationSource =
        new Xenon1tCalibrationSource(pCalibrationSource, m_pWaterLogicalVolume,
                                     Source_X, Source_Y, Source_Z, true, "", "");
  } else {
    // Global Positions obtained from macro (assuming that position is given
    // relative to lab co-ordinate frame)
    Source_X = GetGeometryParameter("Source_x");
    Source_Y = GetGeometryParameter("Source_y");
    Source_Z = GetGeometryParameter("Source_z") + dOuterCryostatOffsetZ -
               dGlobalOffset_Z + dWT_thickness;
    m_pCalibrationSource = new Xenon1tCalibrationSource(
        pCalibrationSource, m_pWaterLogicalVolume, Source_X, Source_Y, Source_Z,
        true, pIBeltCavity, pTunsgtenPlateHole);
  }
}

// SERENA
void Xenon1tDetectorConstruction::ConstructCablesPipe() {
  const G4double dCablesPipeBaseInnerRadius =
      GetGeometryParameter("CablesPipeBaseInnerRadius");
  const G4double dCablesPipeBaseOuterRadius =
      GetGeometryParameter("CablesPipeBaseOuterRadius");
  const G4double dCablesPipeBaseHalfHeight =
      0.5 * GetGeometryParameter("CablesPipeBaseHeight");
  const G4double dCablesPipeBaseZOffset =
      GetGeometryParameter("OuterCryostatDomeOuterHeight") +
      GetGeometryParameter("OuterCryostatCylinderHeight") +
      GetGeometryParameter("OuterCryostatOffsetZ") +
      0.5 * GetGeometryParameter("TankOffset");

  double dCablePipe_tilt_angle = GetGeometryParameter("CablePipe_tilt_angle");
  G4RotationMatrix *rot0 = new G4RotationMatrix();
  rot0->rotateY(dCablePipe_tilt_angle);

  const G4double dCablesPipeInnerRadius =
      GetGeometryParameter("CablesPipeInnerRadius");
  const G4double dCablesPipeOuterRadius =
      GetGeometryParameter("CablesPipeOuterRadius");
  const G4double dCablesPipeHalfHeight =
      0.5 * GetGeometryParameter("CablesPipeHeight");
  const G4double dCablesPipeZOffset =
      dCablesPipeBaseZOffset +
      dCablesPipeHalfHeight * sin(dCablePipe_tilt_angle - 90 * deg);
  const G4double dCablesPipeXOffset =
      GetGeometryParameter("CablesPipeBaseOuterRadius") * 0.5 +
      GetGeometryParameter("WaterTankInnerRadius") * 0.5;

  G4Material *SS304LSteel = G4Material::GetMaterial("SS304LSteel");
  G4Material *Air = G4Material::GetMaterial("G4_AIR");

  // SOLID VOLUMES
  G4Tubs *pCablesPipeBaseSolid =
      new G4Tubs("CablesPipeBaseSolid", dCablesPipeBaseInnerRadius,
                 dCablesPipeBaseOuterRadius, dCablesPipeBaseHalfHeight,
                 0. * deg, 360. * deg);
  G4Tubs *pCablesPipeSolid =
      new G4Tubs("CablesPipeSolid", 0, dCablesPipeOuterRadius,
                 dCablesPipeHalfHeight, 0. * deg, 360. * deg);
  G4Tubs *pCablesPipeAirSolid =
      new G4Tubs("CablesPipeAirSolid", 0, dCablesPipeInnerRadius,
                 dCablesPipeHalfHeight, 0. * deg, 360. * deg);

  // LOGICAL VOLUMES
  m_pCablesPipeBaseLogicalVolume = new G4LogicalVolume(
      pCablesPipeBaseSolid, SS304LSteel, "CablesPipeBaseLogical", 0, 0, 0);
  m_pCablesPipeLogicalVolume = new G4LogicalVolume(
      pCablesPipeSolid, SS304LSteel, "CablesPipeLogical", 0, 0, 0);
  m_pCablesPipeAirLogicalVolume = new G4LogicalVolume(
      pCablesPipeAirSolid, Air, "CablesPipeAirLogical", 0, 0, 0);

  // PHYSICAL VOLUMES
  m_pCablesPipeBasePhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(0, 0, dCablesPipeBaseZOffset),
      m_pCablesPipeBaseLogicalVolume, "PIPEs_CablesPipeBasePhysical",
      m_pWaterLogicalVolume, false, 0);
  m_pCablesPipePhysicalVolume = new G4PVPlacement(
      rot0, G4ThreeVector(dCablesPipeXOffset, 0, dCablesPipeZOffset),
      m_pCablesPipeLogicalVolume, "PIPEs_CablesPipePhysical",
      m_pWaterLogicalVolume, false, 0);
  m_pCablesPipeAirPhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(0, 0, 0), m_pCablesPipeAirLogicalVolume,
      "PIPEs_CablesPipeAirPhysical", m_pCablesPipeLogicalVolume, false, 0);

  // VISUALIZATION ATTRIBUTES
  G4Colour hCablesPipeBaseColor(0.500, 0.500, 0.500, 0.1);
  G4VisAttributes *pCablesPipeBaseVisAtt =
      new G4VisAttributes(hCablesPipeBaseColor);
  pCablesPipeBaseVisAtt->SetVisibility(false);
  m_pCablesPipeBaseLogicalVolume->SetVisAttributes(pCablesPipeBaseVisAtt);
  m_pCablesPipeLogicalVolume->SetVisAttributes(pCablesPipeBaseVisAtt);

  G4Colour hAirColor(0, 1., 1.);
  G4VisAttributes *pCablesPipeAirVisAtt = new G4VisAttributes(hAirColor);
  pCablesPipeAirVisAtt->SetVisibility(false);
  m_pCablesPipeAirLogicalVolume->SetVisAttributes(pCablesPipeAirVisAtt);
}

void Xenon1tDetectorConstruction::ConstructPipe() {
  G4Material *SS316Ti = G4Material::GetMaterial("SS316Ti");
  // G4Material *Air = G4Material::GetMaterial("G4_AIR");
  G4Material *Vacuum = G4Material::GetMaterial("Vacuum");
  G4Material *Torlon = G4Material::GetMaterial("Torlon");

  // G4double cryo_offset = m_hGeometryParameters["OuterCryostatOffsetZ"];
  G4double cryo_offset = 530. * mm;  // MS180209 hard-coded to the
                                     // OuterCryostatOffsetZ of the 1T Cryostat
                                     // (also for the nT version)

  // small pipe paramters
  G4double Rmax_cylinder_small_pipe =
      GetGeometryParameter("Rmax_cylinder_small_pipe");
  G4double Rmin_cylinder_small_pipe =
      GetGeometryParameter("Rmin_cylinder_small_pipe");
  G4double R_base_small = GetGeometryParameter("flange_radius_small_pipe");
  G4double base_height_small = GetGeometryParameter("flange_height_small_pipe");
  G4double cylinder_height_small_pipe =
      GetGeometryParameter("cylinder_height_small_pipe");
  G4double cylinder_long_height_small_pipe =
      GetGeometryParameter("cylinder_long_height_small_pipe");
  G4double torus_radius_small_pipe =
      GetGeometryParameter("torus_radius_small_pipe");

  // central pipe parameters
  G4double R_max_tolon = GetGeometryParameter("R_max_tolon");
  G4double R_min_tolon = GetGeometryParameter("R_min_tolon");
  G4double h_tolon = GetGeometryParameter("h_tolon");
  G4double Rmax_cylinder =
      GetGeometryParameter("Rmax_cylinder_external_central_pipe");
  G4double Rmin_cylinder =
      GetGeometryParameter("Rmin_cylinder_external_central_pipe");
  G4double Rmax_cylinder_internal =
      GetGeometryParameter("Rmax_cylinder_internal_central_big_pipe");
  G4double Rmin_cylinder_internal =
      GetGeometryParameter("Rmin_cylinder_internal_central_big_pipe");
  G4double Rmax_cylinder_internal_1 =
      GetGeometryParameter("Rmax_cylinder_internal_central_pipe_1");
  G4double Rmin_cylinder_internal_1 =
      GetGeometryParameter("Rmin_cylinder_internal_central_pipe_1");
  G4double Rmax_cylinder_internal_2 =
      GetGeometryParameter("Rmax_cylinder_internal_central_pipe_2");
  G4double Rmin_cylinder_internal_2 =
      GetGeometryParameter("Rmin_cylinder_internal_central_pipe_2");

  G4double Rmax_cylinder_internal_3 =
      GetGeometryParameter("Rmax_cylinder_internal_central_pipe_3");
  G4double Rmin_cylinder_internal_3 =
      GetGeometryParameter("Rmin_cylinder_internal_central_pipe_3");
  G4double Rmax_cylinder_internal_4 =
      GetGeometryParameter("Rmax_cylinder_internal_central_pipe_4");
  G4double Rmin_cylinder_internal_4 =
      GetGeometryParameter("Rmin_cylinder_internal_central_pipe_4");
  G4double Rmax_cylinder_internal_5 =
      GetGeometryParameter("Rmax_cylinder_internal_central_pipe_5");
  G4double Rmin_cylinder_internal_5 =
      GetGeometryParameter("Rmin_cylinder_internal_central_pipe_5");

  G4double cylinder_height =
      GetGeometryParameter("cylinder_height_central_pipe");
  G4double cylinder_height_low = GetGeometryParameter("cylinder_height_low");

  G4double torus_angle = GetGeometryParameter("torus_spanned_angle");
  G4double torus_swept_radius = GetGeometryParameter("torus_radius");
  G4double torus_swept_radius_internal5 =
      GetGeometryParameter("torus_radius_internal_5");
  G4double torus_swept_radius_internal4 =
      GetGeometryParameter("torus_radius_internal_4");
  G4double torus_swept_radius_internal3 =
      GetGeometryParameter("torus_radius_internal_3");

  G4double Rmax_cylinder_tilted = Rmax_cylinder;
  G4double Rmin_cylinder_tilted = Rmin_cylinder;

  G4double cylinder_tilted_height_long =
      GetGeometryParameter("cylinder_tilted_long_height_central_pipe");

  G4double base_height = GetGeometryParameter("flange_height");
  G4double base_height_internal =
      GetGeometryParameter("flange_height_internal");
  G4double R_base = GetGeometryParameter("flange_radius");
  G4double R_base_min =
      GetGeometryParameter("flange_radius") -
      20. * mm;  //-20.*mm is the distance between the flange and the plates
  G4double R_base_internal = GetGeometryParameter("internal_big_flange_radius");
  G4double R_base_internal_1 = GetGeometryParameter("internal_flange_radius_1");
  G4double R_base_internal_2 = GetGeometryParameter("internal_flange_radius_2");
  G4double R_base_internal_3 = GetGeometryParameter("internal_flange_radius_3");
  G4double R_base_internal_4 = GetGeometryParameter("internal_flange_radius_4");
  G4double R_base_internal_5 = GetGeometryParameter("internal_flange_radius_5");

  G4double x_offset_internal_1 = GetGeometryParameter("x_offset_internal_1");
  G4double x_offset_internal_2 = GetGeometryParameter("x_offset_internal_2");
  G4double x_offset_internal_3 = GetGeometryParameter("x_offset_internal_3");
  G4double x_offset_internal_4 = GetGeometryParameter("x_offset_internal_4");
  G4double x_offset_internal_5 = GetGeometryParameter("x_offset_internal_5");

  G4double y_offset_internal_1 = GetGeometryParameter("y_offset_internal_1");
  G4double y_offset_internal_2 = GetGeometryParameter("y_offset_internal_2");
  G4double y_offset_internal_3 = GetGeometryParameter("y_offset_internal_3");
  G4double y_offset_internal_4 = GetGeometryParameter("y_offset_internal_4");
  G4double y_offset_internal_5 = GetGeometryParameter("y_offset_internal_5");

  G4double R_small_stain = GetGeometryParameter("R_small_stain");
  G4double height_small_stain = GetGeometryParameter("height_small_stain");

  G4double R_plate = GetGeometryParameter("R_plate");
  G4double h_plate = GetGeometryParameter("h_plate");
  G4double h_plate_low = GetGeometryParameter("h_plate_low");

  G4double z_pipe_box = GetGeometryParameter("z_pipe_box");

  G4double y_pipe_box_1 = GetGeometryParameter("y_pipe_box_1");
  G4double z_pipe_box_1 = GetGeometryParameter("z_pipe_box_1");

  G4double R_cyl_screw_1 = GetGeometryParameter("R_cyl_screw_1");
  G4double R_cyl_screw_2 = GetGeometryParameter("R_cyl_screw_2");
  G4double height_cyl_screw_1 = GetGeometryParameter("height_cyl_screw_1");
  G4double height_cyl_screw_2 = GetGeometryParameter("height_cyl_screw_2");

  // small pipe

  G4Tubs *pcylindersmallpipe =
      new G4Tubs("Cylinder_external_small", 0., Rmax_cylinder_small_pipe,
                 cylinder_height_small_pipe, 0. * deg, 360. * deg);
  G4Tubs *pcylinder_smallpipeair =
      new G4Tubs("Cylinder_vacuum_small", 0., Rmin_cylinder_small_pipe,
                 cylinder_height_small_pipe, 0. * deg, 360. * deg);
  G4Tubs *pcylinderbasesmallpipe =
      new G4Tubs("base_external_small", Rmax_cylinder_small_pipe, R_base_small,
                 base_height_small, 0. * deg, 360. * deg);
  G4Torus *ptorus_external_small = new G4Torus(
      "torus_external_small", Rmin_cylinder_small_pipe,
      Rmax_cylinder_small_pipe, torus_radius_small_pipe, 0. * deg, 85. * deg);
  G4Torus *ptorus_smallpipe_air = new G4Torus(
      "torus_vacuum_small", 0., Rmin_cylinder_small_pipe - 0.001 * mm,
      torus_radius_small_pipe, 0. * deg, 85. * deg);

  G4Tubs *pcylinder_long_small_pipe =
      new G4Tubs("Cylinder_long_small", 0., Rmax_cylinder_small_pipe,
                 cylinder_long_height_small_pipe, 0. * deg, 360. * deg);
  G4Tubs *pcylinder_long_air_small_pipe =
      new G4Tubs("Cylinder_long_small_air", 0., Rmin_cylinder_small_pipe,
                 cylinder_long_height_small_pipe, 0. * deg, 360. * deg);

  // solid of the central pipe
  G4Tubs *pcylindertolon =
      new G4Tubs("Cylinder_external_tolon", R_min_tolon, R_max_tolon, h_tolon,
                 0. * deg, 360. * deg);
  G4Tubs *pcylinderexternaltubs =
      new G4Tubs("Cylinder_external_SS", 0., Rmax_cylinder, cylinder_height,
                 0. * deg, 360. * deg);
  G4Tubs *pcylinder_air = new G4Tubs("Cylinder_vacuum", 0., Rmin_cylinder,
                                     cylinder_height, 0. * deg, 360. * deg);
  G4Tubs *pcylinder_internal =
      new G4Tubs("Cylinder_internal_SS", Rmin_cylinder_internal,
                 Rmax_cylinder_internal, cylinder_height, 0. * deg, 360. * deg);
  G4Tubs *pcylinder_internal_1 = new G4Tubs(
      "Cylinder_internal_SS_1", Rmin_cylinder_internal_1,
      Rmax_cylinder_internal_1, cylinder_height, 0. * deg, 360. * deg);
  G4Tubs *pcylinder_internal_2 = new G4Tubs(
      "Cylinder_internal_SS_2", Rmin_cylinder_internal_2,
      Rmax_cylinder_internal_2, cylinder_height, 0. * deg, 360. * deg);
  G4Tubs *pcylinder_internal_3 = new G4Tubs(
      "Cylinder_internal_SS_3", Rmin_cylinder_internal_3,
      Rmax_cylinder_internal_3, cylinder_height, 0. * deg, 360. * deg);
  G4Tubs *pcylinder_internal_4 = new G4Tubs(
      "Cylinder_internal_SS_4", Rmin_cylinder_internal_4,
      Rmax_cylinder_internal_4, cylinder_height, 0. * deg, 360. * deg);
  G4Tubs *pcylinder_internal_5 = new G4Tubs(
      "Cylinder_internal_SS_5", Rmin_cylinder_internal_5,
      Rmax_cylinder_internal_5, cylinder_height, 0. * deg, 360. * deg);

  G4Tubs *pcylinderexternal_low =
      new G4Tubs("Cylinder_external_SS", 0., Rmax_cylinder, cylinder_height_low,
                 0. * deg, 360. * deg);
  G4Tubs *pcylinder_air_low =
      new G4Tubs("Cylinder_vacuum", 0., Rmin_cylinder, cylinder_height_low,
                 0. * deg, 360. * deg);
  G4Tubs *pcylinder_internal_low = new G4Tubs(
      "Cylinder_internal_SS", Rmin_cylinder_internal, Rmax_cylinder_internal,
      cylinder_height_low, 0. * deg, 360. * deg);
  G4Tubs *pcylinder_internal_low_1 = new G4Tubs(
      "Cylinder_internal_SS_1", Rmin_cylinder_internal_1,
      Rmax_cylinder_internal_1, cylinder_height_low, 0. * deg, 360. * deg);
  G4Tubs *pcylinder_internal_low_2 = new G4Tubs(
      "Cylinder_internal_SS_2", Rmin_cylinder_internal_2,
      Rmax_cylinder_internal_2, cylinder_height_low, 0. * deg, 360. * deg);
  G4Tubs *pcylinder_internal_low_3 = new G4Tubs(
      "Cylinder_internal_SS_3", Rmin_cylinder_internal_3,
      Rmax_cylinder_internal_3, cylinder_height_low, 0. * deg, 360. * deg);
  G4Tubs *pcylinder_internal_low_4 = new G4Tubs(
      "Cylinder_internal_SS_4", Rmin_cylinder_internal_4,
      Rmax_cylinder_internal_4, cylinder_height_low, 0. * deg, 360. * deg);
  G4Tubs *pcylinder_internal_low_5 = new G4Tubs(
      "Cylinder_internal_SS_5", Rmin_cylinder_internal_5,
      Rmax_cylinder_internal_5, cylinder_height_low, 0. * deg, 360. * deg);

  G4Torus *ptorus_external =
      new G4Torus("torus_external", Rmin_cylinder, Rmax_cylinder,
                  torus_swept_radius, 0. * deg, 85. * deg);
  // G4Tubs  *ptorus_small_stain = new G4Torus("torus_external", Rmax_cylinder,
  // R_small_stain, height_small_stain, 0.*deg, 360.*deg);
  G4Torus *ptorus_air =
      new G4Torus("torus_vacuum", 0., Rmin_cylinder - 0.001 * mm,
                  torus_swept_radius, 0. * deg, 85. * deg);
  G4Torus *ptorus_internal = new G4Torus(
      "torus_Internal", Rmin_cylinder_internal, Rmax_cylinder_internal,
      torus_swept_radius, 0. * deg, 85. * deg);

  G4Torus *ptorus_air_internal =
      new G4Torus("torus_vacuum_Internal", 0., Rmin_cylinder_internal,
                  torus_swept_radius, 0. * deg, 85. * deg);
  G4Torus *ptorus_internal_1 = new G4Torus(
      "torus_Internal_1", Rmin_cylinder_internal_1, Rmax_cylinder_internal_1,
      torus_swept_radius, 0. * deg, 85. * deg);
  G4Torus *ptorus_internal_2 = new G4Torus(
      "torus_Internal_2", Rmin_cylinder_internal_2, Rmax_cylinder_internal_2,
      torus_swept_radius, 0. * deg, 85. * deg);
  G4Torus *ptorus_internal_3 = new G4Torus(
      "torus_Internal_3", Rmin_cylinder_internal_3, Rmax_cylinder_internal_3,
      torus_swept_radius_internal3, 0. * deg, 85. * deg);
  G4Torus *ptorus_internal_4 = new G4Torus(
      "torus_Internal_4", Rmin_cylinder_internal_4, Rmax_cylinder_internal_4,
      torus_swept_radius_internal4, 0. * deg, 85. * deg);
  G4Torus *ptorus_internal_5 = new G4Torus(
      "torus_Internal_5", Rmin_cylinder_internal_5, Rmax_cylinder_internal_5,
      torus_swept_radius_internal5, 0. * deg, 85. * deg);

  G4Tubs *pcylinder_tilted_external_long =
      new G4Tubs("Cylinder_external_tilted_long", 0., Rmax_cylinder_tilted,
                 cylinder_tilted_height_long, 0. * deg, 360. * deg);
  G4Tubs *pcylinder_tilted_air_long = new G4Tubs(
      "Cylinder_external_tilted_vacuum_long", 0., Rmin_cylinder_tilted,
      cylinder_tilted_height_long, 0. * deg, 360. * deg);
  G4Tubs *pcylinder_tilted_internal_long =
      new G4Tubs("Cylinder_internal_SS_long", Rmin_cylinder_internal,
                 Rmax_cylinder_internal, cylinder_tilted_height_long, 0. * deg,
                 360. * deg);
  G4Tubs *pcylinder_tilted_internal_long_1 =
      new G4Tubs("Cylinder_internal_SS_long_1", Rmin_cylinder_internal_1,
                 Rmax_cylinder_internal_1, cylinder_tilted_height_long,
                 0. * deg, 360. * deg);
  G4Tubs *pcylinder_tilted_internal_long_2 =
      new G4Tubs("Cylinder_internal_SS_long_2", Rmin_cylinder_internal_2,
                 Rmax_cylinder_internal_2, cylinder_tilted_height_long,
                 0. * deg, 360. * deg);
  G4Tubs *pcylinder_tilted_internal_long_3 =
      new G4Tubs("Cylinder_internal_SS_long_3", Rmin_cylinder_internal_3,
                 Rmax_cylinder_internal_3, cylinder_tilted_height_long,
                 0. * deg, 360. * deg);
  G4Tubs *pcylinder_tilted_internal_long_4 =
      new G4Tubs("Cylinder_internal_SS_long_4", Rmin_cylinder_internal_4,
                 Rmax_cylinder_internal_4, cylinder_tilted_height_long,
                 0. * deg, 360. * deg);
  G4Tubs *pcylinder_tilted_internal_long_5 =
      new G4Tubs("Cylinder_internal_SS_long_5", Rmin_cylinder_internal_5,
                 Rmax_cylinder_internal_5, cylinder_tilted_height_long,
                 0. * deg, 360. * deg);

  G4Tubs *pcylinderbase = new G4Tubs("flange", R_base_min, R_base, base_height,
                                     0. * deg, 360. * deg);
  G4Tubs *pcylinderbase_internal =
      new G4Tubs("flange_internal", Rmax_cylinder_internal, R_base_internal,
                 base_height_internal, 0. * deg, 360. * deg);
  G4Tubs *pcylinderbase_internal_1 =
      new G4Tubs("flange_internal_1", Rmax_cylinder_internal_1,
                 R_base_internal_1, base_height_internal, 0. * deg, 360. * deg);
  G4Tubs *pcylinderbase_internal_2 =
      new G4Tubs("flange_internal_2", Rmax_cylinder_internal_2,
                 R_base_internal_2, base_height_internal, 0. * deg, 360. * deg);
  G4Tubs *pcylinderbase_internal_3 =
      new G4Tubs("flange_internal_3", Rmax_cylinder_internal_3,
                 R_base_internal_3, base_height_internal, 0. * deg, 360. * deg);
  G4Tubs *pcylinderbase_internal_4 =
      new G4Tubs("flange_internal_4", Rmax_cylinder_internal_4,
                 R_base_internal_4, base_height_internal, 0. * deg, 360. * deg);
  G4Tubs *pcylinderbase_internal_5 =
      new G4Tubs("flange_internal_5", Rmax_cylinder_internal_5,
                 R_base_internal_5, base_height_internal, 0. * deg, 360. * deg);

  G4Tubs *pcylinder_small_stain =
      new G4Tubs("Cylinder_small_stain", Rmax_cylinder, R_small_stain,
                 height_small_stain, 0. * deg, 360. * deg);
  G4Tubs *pcylinder_plate_small =
      new G4Tubs("Cylinder_small_plate", Rmax_cylinder, R_plate, h_plate,
                 0. * deg, 360. * deg);
  G4Tubs *pcylinder_plate_low_small =
      new G4Tubs("Cylinder_small_low_plate", Rmax_cylinder, R_plate,
                 h_plate_low, 0. * deg, 360. * deg);

  G4double anglepipebox = 8.49 * deg;
  G4Tubs *pboxpipe = new G4Tubs("boxpipe", R_plate, R_base_min, z_pipe_box, 0,
                                anglepipebox);  // anglepipebox);

  G4Tubs *pboxpipe1 = new G4Tubs("boxpipe1", R_plate, R_base_min, z_pipe_box,
                                 360. * deg - anglepipebox, anglepipebox);
  G4Tubs *pboxpipe2 = new G4Tubs("boxpipe2", R_plate, R_base_min, z_pipe_box,
                                 90. * deg, anglepipebox);
  G4Tubs *pboxpipe3 = new G4Tubs("boxpipe3", R_plate, R_base_min, z_pipe_box,
                                 90. * deg - anglepipebox, anglepipebox);
  G4Tubs *pboxpipe4 = new G4Tubs("boxpipe4", R_plate, R_base_min, z_pipe_box,
                                 180. * deg, anglepipebox);
  G4Tubs *pboxpipe5 = new G4Tubs("boxpipe5", R_plate, R_base_min, z_pipe_box,
                                 180. * deg - anglepipebox, anglepipebox);
  G4Tubs *pboxpipe6 = new G4Tubs("boxpipe6", R_plate, R_base_min, z_pipe_box,
                                 270. * deg, anglepipebox);
  G4Tubs *pboxpipe7 = new G4Tubs("boxpipe7", R_plate, R_base_min, z_pipe_box,
                                 270. * deg - anglepipebox, anglepipebox);

  G4double anglepipeboxsmall = 1.2 * deg;
  G4Tubs *pboxpipesmall1 = new G4Tubs(
      "boxpipesmall1", Rmax_cylinder, Rmax_cylinder + y_pipe_box_1,
      z_pipe_box_1, anglepipebox + anglepipeboxsmall, anglepipeboxsmall);
  G4Tubs *pboxpipesmall2 =
      new G4Tubs("boxpipesmall2", Rmax_cylinder, Rmax_cylinder + y_pipe_box_1,
                 z_pipe_box_1, 360. * deg - anglepipebox - anglepipeboxsmall,
                 anglepipeboxsmall);
  G4Tubs *pboxpipesmall3 =
      new G4Tubs("boxpipesmall3", Rmax_cylinder, Rmax_cylinder + y_pipe_box_1,
                 z_pipe_box_1, 45. * deg, anglepipeboxsmall);
  G4Tubs *pboxpipesmall4 =
      new G4Tubs("boxpipesmall4", Rmax_cylinder, Rmax_cylinder + y_pipe_box_1,
                 z_pipe_box_1, 80. * deg, anglepipeboxsmall);
  G4Tubs *pboxpipesmall5 =
      new G4Tubs("boxpipesmall5", Rmax_cylinder, Rmax_cylinder + y_pipe_box_1,
                 z_pipe_box_1, 315. * deg, anglepipeboxsmall);
  G4Tubs *pboxpipesmall6 =
      new G4Tubs("boxpipesmall6", Rmax_cylinder, Rmax_cylinder + y_pipe_box_1,
                 z_pipe_box_1, 285. * deg, anglepipeboxsmall);
  G4Tubs *pboxpipesmall7 =
      new G4Tubs("boxpipesmall7", Rmax_cylinder, Rmax_cylinder + y_pipe_box_1,
                 z_pipe_box_1, 180. * deg + anglepipebox - anglepipeboxsmall,
                 anglepipeboxsmall);
  G4Tubs *pboxpipesmall8 =
      new G4Tubs("boxpipesmall8", Rmax_cylinder, Rmax_cylinder + y_pipe_box_1,
                 z_pipe_box_1, 180. * deg - anglepipebox - anglepipeboxsmall,
                 anglepipeboxsmall);
  G4Tubs *pboxpipesmall9 =
      new G4Tubs("boxpipesmall9", Rmax_cylinder, Rmax_cylinder + y_pipe_box_1,
                 z_pipe_box_1, 225. * deg, anglepipeboxsmall);
  G4Tubs *pboxpipesmall10 =
      new G4Tubs("boxpipesmall10", Rmax_cylinder, Rmax_cylinder + y_pipe_box_1,
                 z_pipe_box_1, 260. * deg, anglepipeboxsmall);
  G4Tubs *pboxpipesmall11 =
      new G4Tubs("boxpipesmall11", Rmax_cylinder, Rmax_cylinder + y_pipe_box_1,
                 z_pipe_box_1, 135. * deg, anglepipeboxsmall);
  G4Tubs *pboxpipesmall12 =
      new G4Tubs("boxpipesmall12", Rmax_cylinder, Rmax_cylinder + y_pipe_box_1,
                 z_pipe_box_1, 100. * deg, anglepipeboxsmall);

  G4Tubs *pcylinderscrew1 = new G4Tubs(
      "screw1", 0, R_cyl_screw_1, height_cyl_screw_1, 0. * deg, 360. * deg);
  G4Tubs *pcylinderscrew2 = new G4Tubs(
      "screw2", 0, R_cyl_screw_2, height_cyl_screw_2, 0. * deg, 360. * deg);
  G4ThreeVector screwunionvec(0, 0, height_cyl_screw_1 + height_cyl_screw_2);
  G4UnionSolid *screwunion = new G4UnionSolid(
      "screwunion", pcylinderscrew1, pcylinderscrew2, 0, screwunionvec);

  G4RotationMatrix *Rotflange = new G4RotationMatrix;
  Rotflange->rotateX(90. * deg);
  G4RotationMatrix *Rotflangeopp = new G4RotationMatrix;
  Rotflangeopp->rotateX(-90. * deg);
  G4ThreeVector screwoffset(0, R_base + height_cyl_screw_1, 0);
  G4RotationMatrix *Rotflange1 = new G4RotationMatrix;
  Rotflange1->rotateY(-90. * deg);
  G4ThreeVector screwoffset1(R_base + height_cyl_screw_1, 0, 0);
  G4RotationMatrix *Rotflangeopp1 = new G4RotationMatrix;
  Rotflangeopp1->rotateY(90. * deg);

  G4UnionSolid *flangescrew = new G4UnionSolid(
      "flangescrew", pcylinderbase, screwunion, Rotflange, screwoffset);
  flangescrew = new G4UnionSolid("flangescrew", flangescrew, screwunion,
                                 Rotflangeopp, -screwoffset);
  flangescrew = new G4UnionSolid("flangescrew", flangescrew, screwunion,
                                 Rotflange1, screwoffset1);
  flangescrew = new G4UnionSolid("flangescrew", flangescrew, screwunion,
                                 Rotflangeopp1, -screwoffset1);

  G4ThreeVector plateoffset(0, 0, base_height + 34. * mm + h_plate);
  G4ThreeVector platelowoffset(0, 0, base_height + 106.5 * mm + h_plate_low);

  flangescrew = new G4UnionSolid("flangescrew", flangescrew,
                                 pcylinder_plate_small, 0, plateoffset);
  flangescrew = new G4UnionSolid("flangescrew", flangescrew,
                                 pcylinder_plate_small, 0, -plateoffset);
  flangescrew = new G4UnionSolid("flangescrew", flangescrew,
                                 pcylinder_plate_low_small, 0, -platelowoffset);

  G4ThreeVector boxoffset(0, 0, -base_height + 10. * mm);

  G4ThreeVector boxoffset1(0, 0, base_height + 5. * mm);

  G4ThreeVector smallboxoffset(0, 0, base_height + 40. * mm + z_pipe_box_1);

  flangescrew =
      new G4UnionSolid("flangescrew", flangescrew, pboxpipe, 0, boxoffset1);
  flangescrew =
      new G4UnionSolid("flangescrew", flangescrew, pboxpipe1, 0, boxoffset1);
  flangescrew =
      new G4UnionSolid("flangescrew", flangescrew, pboxpipe2, 0, boxoffset);
  flangescrew =
      new G4UnionSolid("flangescrew", flangescrew, pboxpipe3, 0, boxoffset);
  flangescrew =
      new G4UnionSolid("flangescrew", flangescrew, pboxpipe4, 0, boxoffset1);
  flangescrew =
      new G4UnionSolid("flangescrew", flangescrew, pboxpipe5, 0, boxoffset1);
  flangescrew =
      new G4UnionSolid("flangescrew", flangescrew, pboxpipe6, 0, boxoffset);
  flangescrew =
      new G4UnionSolid("flangescrew", flangescrew, pboxpipe7, 0, boxoffset);

  flangescrew = new G4UnionSolid("flangescrew", flangescrew, pboxpipesmall1, 0,
                                 smallboxoffset);
  flangescrew = new G4UnionSolid("flangescrew", flangescrew, pboxpipesmall2, 0,
                                 smallboxoffset);
  flangescrew = new G4UnionSolid("flangescrew", flangescrew, pboxpipesmall3, 0,
                                 smallboxoffset);
  flangescrew = new G4UnionSolid("flangescrew", flangescrew, pboxpipesmall4, 0,
                                 smallboxoffset);
  flangescrew = new G4UnionSolid("flangescrew", flangescrew, pboxpipesmall5, 0,
                                 smallboxoffset);
  flangescrew = new G4UnionSolid("flangescrew", flangescrew, pboxpipesmall6, 0,
                                 smallboxoffset);
  flangescrew = new G4UnionSolid("flangescrew", flangescrew, pboxpipesmall7, 0,
                                 smallboxoffset);
  flangescrew = new G4UnionSolid("flangescrew", flangescrew, pboxpipesmall8, 0,
                                 smallboxoffset);
  flangescrew = new G4UnionSolid("flangescrew", flangescrew, pboxpipesmall9, 0,
                                 smallboxoffset);
  flangescrew = new G4UnionSolid("flangescrew", flangescrew, pboxpipesmall10, 0,
                                 smallboxoffset);
  flangescrew = new G4UnionSolid("flangescrew", flangescrew, pboxpipesmall11, 0,
                                 smallboxoffset);
  flangescrew = new G4UnionSolid("flangescrew", flangescrew, pboxpipesmall12, 0,
                                 smallboxoffset);

  flangescrew = new G4UnionSolid("flangescrew", flangescrew, pboxpipesmall1, 0,
                                 -smallboxoffset);
  flangescrew = new G4UnionSolid("flangescrew", flangescrew, pboxpipesmall2, 0,
                                 -smallboxoffset);
  flangescrew = new G4UnionSolid("flangescrew", flangescrew, pboxpipesmall3, 0,
                                 -smallboxoffset);
  flangescrew = new G4UnionSolid("flangescrew", flangescrew, pboxpipesmall4, 0,
                                 -smallboxoffset);
  flangescrew = new G4UnionSolid("flangescrew", flangescrew, pboxpipesmall5, 0,
                                 -smallboxoffset);
  flangescrew = new G4UnionSolid("flangescrew", flangescrew, pboxpipesmall6, 0,
                                 -smallboxoffset);
  flangescrew = new G4UnionSolid("flangescrew", flangescrew, pboxpipesmall7, 0,
                                 -smallboxoffset);
  flangescrew = new G4UnionSolid("flangescrew", flangescrew, pboxpipesmall8, 0,
                                 -smallboxoffset);
  flangescrew = new G4UnionSolid("flangescrew", flangescrew, pboxpipesmall9, 0,
                                 -smallboxoffset);
  flangescrew = new G4UnionSolid("flangescrew", flangescrew, pboxpipesmall10, 0,
                                 -smallboxoffset);
  flangescrew = new G4UnionSolid("flangescrew", flangescrew, pboxpipesmall11, 0,
                                 -smallboxoffset);
  flangescrew = new G4UnionSolid("flangescrew", flangescrew, pboxpipesmall12, 0,
                                 -smallboxoffset);

  m_pPipeCylinderSmallSSLogicalVolume =
      new G4LogicalVolume(pcylindersmallpipe, SS316Ti, "Cylinder_SS");
  m_pPipeCylinderAirSmallLogicalVolume =
      new G4LogicalVolume(pcylinder_smallpipeair, Vacuum, "Cylinder_vacuum");
  m_pPipeTorusSmallSSLogicalVolume =
      new G4LogicalVolume(ptorus_external_small, SS316Ti, "Torus_SS");
  m_pPipeTorusAirSmallLogicalVolume =
      new G4LogicalVolume(ptorus_smallpipe_air, Vacuum, "Torus_vacuum");
  m_pPipeCylinderTiltedLongSmallSSLogicalVolume = new G4LogicalVolume(
      pcylinder_long_small_pipe, SS316Ti, "Cylinder_Tilted_SS");
  m_pPipeCylinderTiltedLongAirSmallLogicalVolume = new G4LogicalVolume(
      pcylinder_long_air_small_pipe, Vacuum, "Cylinder_Tilted_vacuum");
  m_pPipeBaseSmallLogicalVolume =
      new G4LogicalVolume(pcylinderbasesmallpipe, SS316Ti, "Base");

  // logical volume cryogenics pipe

  m_pPipeTolonLogicalVolume =
      new G4LogicalVolume(pcylindertolon, Torlon, "Cylinder_SS");
  m_pPipeCylinderSSLogicalVolume =
      new G4LogicalVolume(pcylinderexternaltubs, SS316Ti, "Cylinder_SS");
  m_pPipeCylinderAirLogicalVolume =
      new G4LogicalVolume(pcylinder_air, Vacuum, "Cylinder_vacuum");
  m_pPipeCylinderInternalSSLogicalVolume =
      new G4LogicalVolume(pcylinder_internal, SS316Ti, "Cylinder_SS");
  m_pPipeCylinderInternalSSLogicalVolume_1 =
      new G4LogicalVolume(pcylinder_internal_1, SS316Ti, "Cylinder_SS_1");
  m_pPipeCylinderInternalSSLogicalVolume_2 =
      new G4LogicalVolume(pcylinder_internal_2, SS316Ti, "Cylinder_SS_2");
  m_pPipeCylinderInternalSSLogicalVolume_3 =
      new G4LogicalVolume(pcylinder_internal_3, SS316Ti, "Cylinder_SS_3");
  m_pPipeCylinderInternalSSLogicalVolume_4 =
      new G4LogicalVolume(pcylinder_internal_4, SS316Ti, "Cylinder_SS_4");
  m_pPipeCylinderInternalSSLogicalVolume_5 =
      new G4LogicalVolume(pcylinder_internal_5, SS316Ti, "Cylinder_SS_5");

  m_pPipeCylinderLowSSLogicalVolume =
      new G4LogicalVolume(pcylinderexternal_low, SS316Ti, "Cylinder_SS");
  m_pPipeCylinderLowAirLogicalVolume =
      new G4LogicalVolume(pcylinder_air_low, Vacuum, "Cylinder_vacuum");
  m_pPipeCylinderInternalLowSSLogicalVolume =
      new G4LogicalVolume(pcylinder_internal_low, SS316Ti, "Cylinder_SS");
  m_pPipeCylinderInternalLowSSLogicalVolume_1 =
      new G4LogicalVolume(pcylinder_internal_low_1, SS316Ti, "Cylinder_SS_1");
  m_pPipeCylinderInternalLowSSLogicalVolume_2 =
      new G4LogicalVolume(pcylinder_internal_low_2, SS316Ti, "Cylinder_SS_2");
  m_pPipeCylinderInternalLowSSLogicalVolume_3 =
      new G4LogicalVolume(pcylinder_internal_low_3, SS316Ti, "Cylinder_SS_3");
  m_pPipeCylinderInternalLowSSLogicalVolume_4 =
      new G4LogicalVolume(pcylinder_internal_low_4, SS316Ti, "Cylinder_SS_4");
  m_pPipeCylinderInternalLowSSLogicalVolume_5 =
      new G4LogicalVolume(pcylinder_internal_low_5, SS316Ti, "Cylinder_SS_5");
  m_pPipeCylinderSmallStainSSLogicalVolume = new G4LogicalVolume(
      pcylinder_small_stain, SS316Ti, "Cylinder_Small_stain");

  m_pPipeTorusSSLogicalVolume =
      new G4LogicalVolume(ptorus_external, SS316Ti, "Torus_SS");
  m_pPipeTorusAirLogicalVolume =
      new G4LogicalVolume(ptorus_air, Vacuum, "Torus_vacuum");
  m_pPipeTorusInternalSSLogicalVolume =
      new G4LogicalVolume(ptorus_internal, SS316Ti, "Torus_SS");
  m_pPipeTorusInternalSSLogicalVolume_1 =
      new G4LogicalVolume(ptorus_internal_1, SS316Ti, "Torus_SS");
  m_pPipeTorusInternalSSLogicalVolume_2 =
      new G4LogicalVolume(ptorus_internal_2, SS316Ti, "Torus_SS");
  m_pPipeTorusInternalSSLogicalVolume_3 =
      new G4LogicalVolume(ptorus_internal_3, SS316Ti, "Torus_SS");
  m_pPipeTorusInternalSSLogicalVolume_4 =
      new G4LogicalVolume(ptorus_internal_4, SS316Ti, "Torus_SS");
  m_pPipeTorusInternalSSLogicalVolume_5 =
      new G4LogicalVolume(ptorus_internal_5, SS316Ti, "Torus_SS");
  m_pPipeTorusAirInternalLogicalVolume =
      new G4LogicalVolume(ptorus_air_internal, Vacuum, "Torus_vacuum_internal");

  m_pPipeCylinderTiltedLongSSLogicalVolume = new G4LogicalVolume(
      pcylinder_tilted_external_long, SS316Ti, "Cylinder_Tilted_SS");
  m_pPipeCylinderTiltedLongAirLogicalVolume = new G4LogicalVolume(
      pcylinder_tilted_air_long, Vacuum, "Cylinder_Tilted_vacuum");
  m_pPipeCylinderTiltedLongInternalSSLogicalVolume = new G4LogicalVolume(
      pcylinder_tilted_internal_long, SS316Ti, "Cylinder_Tilted_in_SS");
  m_pPipeCylinderTiltedLongInternalSSLogicalVolume_1 = new G4LogicalVolume(
      pcylinder_tilted_internal_long_1, SS316Ti, "Cylinder_Tilted_in_SS");
  m_pPipeCylinderTiltedLongInternalSSLogicalVolume_2 = new G4LogicalVolume(
      pcylinder_tilted_internal_long_2, SS316Ti, "Cylinder_Tilted_in_SS");
  m_pPipeCylinderTiltedLongInternalSSLogicalVolume_3 = new G4LogicalVolume(
      pcylinder_tilted_internal_long_3, SS316Ti, "Cylinder_Tilted_in_SS");
  m_pPipeCylinderTiltedLongInternalSSLogicalVolume_4 = new G4LogicalVolume(
      pcylinder_tilted_internal_long_4, SS316Ti, "Cylinder_Tilted_in_SS");
  m_pPipeCylinderTiltedLongInternalSSLogicalVolume_5 = new G4LogicalVolume(
      pcylinder_tilted_internal_long_5, SS316Ti, "Cylinder_Tilted_in_SS");

  m_pPipeBaseLogicalVolume = new G4LogicalVolume(flangescrew, SS316Ti, "Base");
  m_pPipeBaseInternalLogicalVolume =
      new G4LogicalVolume(pcylinderbase_internal, SS316Ti, "Base");
  m_pPipeBaseInternal1LogicalVolume =
      new G4LogicalVolume(pcylinderbase_internal_1, SS316Ti, "Base");
  m_pPipeBaseInternal2LogicalVolume =
      new G4LogicalVolume(pcylinderbase_internal_2, SS316Ti, "Base");
  m_pPipeBaseInternal3LogicalVolume =
      new G4LogicalVolume(pcylinderbase_internal_3, SS316Ti, "Base");
  m_pPipeBaseInternal4LogicalVolume =
      new G4LogicalVolume(pcylinderbase_internal_4, SS316Ti, "Base");
  m_pPipeBaseInternal5LogicalVolume =
      new G4LogicalVolume(pcylinderbase_internal_5, SS316Ti, "Base");

  //  m_pPipeBaseHalfLogicalVolume= new G4LogicalVolume(flangescrew2, SS316Ti,
  //  "Base");

  // m_pPipeBaseInternalLogicalVolume= new
  // G4LogicalVolume(pcylinderbase_internal, SS316Ti, "Base");

  // position of the physical volumes of the central pipe.

  G4double x_base_small = 450.;
  G4double y_base_small = 260.;
  G4double z_base_small =
      1180.1 * mm + cryo_offset + base_height_small;  //-3.85
  G4double z_cylinder_small =
      1180.1 * mm + cryo_offset + cylinder_height_small_pipe;
  G4double z_torus_small = z_cylinder_small + cylinder_height_small_pipe;
  G4double y_torus_small = y_base_small - torus_radius_small_pipe;
  G4double y_cylinder_tilted_long_small =
      y_torus_small -
      cylinder_long_height_small_pipe * cos(90. * deg - torus_angle) +
      torus_radius_small_pipe * sin(90. * deg - torus_angle);
  G4double z_cylinder_tilted_long_small =
      z_torus_small +
      cylinder_long_height_small_pipe * sin(90. * deg - torus_angle) +
      torus_radius_small_pipe * cos(90. * deg - torus_angle);

  G4double x_base_low = 0.;
  G4double y_base_low = 0.;
  G4double z_base_low = 1261.1 * mm + cryo_offset + base_height +
                        2. * cylinder_height_low;  //-3.85
  G4double y_cylinder = 0.;
  G4double x_cylinder_external = 0.;
  G4double y_cylinder_external = 0.;
  G4double x_cylinder_internal_1 = x_offset_internal_1;
  G4double y_cylinder_internal_1 = y_offset_internal_1;
  G4double x_cylinder_internal_2 = x_offset_internal_2;
  G4double y_cylinder_internal_2 = y_offset_internal_2;
  G4double x_cylinder_internal_3 = x_offset_internal_3;
  G4double y_cylinder_internal_3 = y_offset_internal_3;
  G4double x_cylinder_internal_4 = x_offset_internal_4;
  G4double y_cylinder_internal_4 = y_offset_internal_4;
  G4double x_cylinder_internal_5 = x_offset_internal_5;
  G4double y_cylinder_internal_5 = y_offset_internal_5;

  G4double z_cylinder =
      1261.1 * mm + cryo_offset + cylinder_height + 2. * cylinder_height_low;
  G4double z_cylinder_low = 1261.1 * mm + cryo_offset + cylinder_height_low;

  G4double z_torus = z_cylinder + cylinder_height;
  G4double y_torus = y_cylinder - torus_swept_radius;
  G4double x_torus = 0.;
  // G4double z_torus_internal_1 = z_cylinder + cylinder_height;
  // G4double y_torus_internal_1 = y_cylinder-torus_swept_radius;
  G4double x_torus_internal_1 = x_offset_internal_1;
  // G4double z_torus_internal_2 = z_cylinder + cylinder_height;
  // G4double y_torus_internal_2 = y_cylinder-torus_swept_radius;
  G4double x_torus_internal_2 = x_offset_internal_2;
  G4double y_torus_internal_3 =
      y_offset_internal_3 +
      (-torus_swept_radius_internal3 +
       torus_swept_radius);  //-Rmax_cylinder_internal_3-(-torus_swept_radius_internal3+torus_swept_radius)*cos(85.*deg);//+torus_swept_radius_internal3-torus_swept_radius;
  G4double x_torus_internal_3 = x_offset_internal_3;

  G4double y_torus_internal_4 =
      y_offset_internal_4 +
      (-torus_swept_radius_internal4 + torus_swept_radius);
  G4double x_torus_internal_4 = x_offset_internal_4;

  G4double y_torus_internal_5 =
      y_offset_internal_5 +
      (-torus_swept_radius_internal5 + torus_swept_radius);
  G4double x_torus_internal_5 = x_offset_internal_5;

  G4RotationMatrix *Rot_torus = new G4RotationMatrix;
  Rot_torus->rotateX(270. * deg);
  Rot_torus->rotateY(270. * deg);

  G4double y_cylinder_tilted_long =
      y_torus - cylinder_tilted_height_long * cos(90. * deg - torus_angle) +
      torus_swept_radius * sin(90. * deg - torus_angle);
  G4double x_cylinder_tilted_long = 0. * mm;
  G4double z_cylinder_tilted_long =
      z_torus + cylinder_tilted_height_long * sin(90. * deg - torus_angle) +
      torus_swept_radius * cos(90. * deg - torus_angle);
  /* G4double y_cylinder_tilted_long_internal_1 = y_torus_internal_1 -
   cylinder_tilted_height_long*cos(90.*deg-torus_angle)+torus_swept_radius*sin(90.*deg-torus_angle);
   G4double x_cylinder_tilted_long_internal_1 = x_offset_internal_1;
   G4double z_cylinder_tilted_long_internal_1 = z_torus_internal_1 +
   cylinder_tilted_height_long*sin(90.*deg-torus_angle)
   +torus_swept_radius*cos(90.*deg-torus_angle);
   G4double y_cylinder_tilted_long_internal_2 = y_torus_internal_2 -
   cylinder_tilted_height_long*cos(90.*deg-torus_angle)+torus_swept_radius*sin(90.*deg-torus_angle);
   G4double x_cylinder_tilted_long_internal_2 = x_offset_internal_2;
   G4double z_cylinder_tilted_long_internal_2 = z_torus_internal_2 +
   cylinder_tilted_height_long*sin(90.*deg-torus_angle)
   +torus_swept_radius*cos(90.*deg-torus_angle);
 */
  G4double y_cylinder_tilted_long_internal_3 =
      y_offset_internal_3;  //-(y_torus_internal_3
  //+torus_swept_radius_internal3*sin(90.*deg-torus_angle));
  // G4double x_cylinder_tilted_long_internal_3 = x_offset_internal_3;
  G4double z_cylinder_tilted_long_internal_3 =
      0.;  //-(torus_swept_radius_internal3*cos(90.*deg-torus_angle));

  G4double y_base_tilt =
      y_cylinder_tilted_long -
      (cylinder_tilted_height_long - 200.) * cos(90. * deg - torus_angle) -
      0.5 * base_height * cos(90. * deg - torus_angle);
  G4double x_base_tilt = 0.;
  G4double z_base_tilt =
      z_cylinder_tilted_long +
      (cylinder_tilted_height_long - 200.) * sin(90. * deg - torus_angle) +
      0.5 * base_height * sin(90. * deg - torus_angle);

  G4RotationMatrix *Rot_cyl = new G4RotationMatrix;
  Rot_cyl->rotateX(-torus_angle);
  G4RotationMatrix *Rot_cyl1 = new G4RotationMatrix;
  Rot_cyl1->rotateX(-torus_angle + 180. * deg);
  G4RotationMatrix *Rot_small_stain = new G4RotationMatrix;
  Rot_small_stain->rotateX(135. * deg);

  // Physical volume central pipe

  m_pPipeTolonPhysicalVolume1 = new G4PVPlacement(
      0, G4ThreeVector(0., 0., -27.5 * mm), m_pPipeTolonLogicalVolume,
      "Pipe_Torlon_external", m_pPipeCylinderAirLogicalVolume, false, 0);

  m_pPipeBaseLowPhysicalVolume =
      new G4PVPlacement(0, G4ThreeVector(x_base_low, y_base_low, z_base_low),
                        m_pPipeBaseLogicalVolume, "Pipe_SS_Flange_Base",
                        m_pWaterLogicalVolume, false, 0);
  m_pPipeCylinderInternalSSPhysicalVolume_1 = new G4PVPlacement(
      0, G4ThreeVector(x_cylinder_internal_1, y_cylinder_internal_1, 0.),
      m_pPipeCylinderInternalSSLogicalVolume_1,
      "Pipe_SS_cylinder_internal_SS_1", m_pPipeCylinderAirLogicalVolume, false,
      0);
  m_pPipeCylinderInternalSSPhysicalVolume_2 = new G4PVPlacement(
      0, G4ThreeVector(x_cylinder_internal_2, y_cylinder_internal_2, 0.),
      m_pPipeCylinderInternalSSLogicalVolume_2,
      "Pipe_SS_cylinder_internal_SS_2", m_pPipeCylinderAirLogicalVolume, false,
      0);
  m_pPipeCylinderInternalSSPhysicalVolume_3 = new G4PVPlacement(
      0, G4ThreeVector(x_cylinder_internal_3, y_cylinder_internal_3, 0.),
      m_pPipeCylinderInternalSSLogicalVolume_3,
      "Pipe_SS_cylinder_internal_SS_3", m_pPipeCylinderAirLogicalVolume, false,
      0);
  m_pPipeCylinderInternalSSPhysicalVolume_4 = new G4PVPlacement(
      0, G4ThreeVector(x_cylinder_internal_4, y_cylinder_internal_4, 0.),
      m_pPipeCylinderInternalSSLogicalVolume_4,
      "Pipe_SS_cylinder_internal_SS_4", m_pPipeCylinderAirLogicalVolume, false,
      0);
  m_pPipeCylinderInternalSSPhysicalVolume_5 = new G4PVPlacement(
      0, G4ThreeVector(x_cylinder_internal_5, y_cylinder_internal_5, 0.),
      m_pPipeCylinderInternalSSLogicalVolume_5,
      "Pipe_SS_cylinder_internal_SS_5", m_pPipeCylinderAirLogicalVolume, false,
      0);
  m_pPipeCylinderInternalSSPhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(0., 0., 0.), m_pPipeCylinderInternalSSLogicalVolume,
      "Pipe_SS_cylinder_internal_SS", m_pPipeCylinderAirLogicalVolume, false,
      0);
  /*   m_pPipeBaseLowInternalPhysicalVolume = new G4PVPlacement(0,
 G4ThreeVector(0.,0.,-cylinder_height+base_height_internal),
 m_pPipeBaseInternalLogicalVolume, "Pipe_Flange_Base_internal",
 m_pPipeCylinderAirLogicalVolume, false, 0);//+52.mm donato indication
       m_pPipeBaseLowInternal1PhysicalVolume = new G4PVPlacement(0,
 G4ThreeVector(x_cylinder_internal_1,y_cylinder_internal_1,-cylinder_height+base_height_internal),
 m_pPipeBaseInternal1LogicalVolume, "Pipe_Flange_Base_internal_1",
 m_pPipeCylinderAirLogicalVolume, false, 0);
 m_pPipeBaseLowInternal2PhysicalVolume = new G4PVPlacement(0,
 G4ThreeVector(x_cylinder_internal_2,y_cylinder_internal_2,-cylinder_height+base_height_internal),
 m_pPipeBaseInternal2LogicalVolume, "Pipe_Flange_Base_internal_2",
 m_pPipeCylinderAirLogicalVolume, false, 0);
       m_pPipeBaseLowInternal3PhysicalVolume = new G4PVPlacement(0,
 G4ThreeVector(x_cylinder_internal_3,y_cylinder_internal_3,-cylinder_height+base_height_internal),
 m_pPipeBaseInternal3LogicalVolume, "Pipe_Flange_Base_internal_3",
 m_pPipeCylinderAirLogicalVolume, false, 0);
 m_pPipeBaseLowInternal4PhysicalVolume = new G4PVPlacement(0,
 G4ThreeVector(x_cylinder_internal_4,y_cylinder_internal_4,-cylinder_height+base_height_internal),
 m_pPipeBaseInternal4LogicalVolume, "Pipe_Flange_Base_internal_4",
 m_pPipeCylinderAirLogicalVolume, false, 0);
 m_pPipeBaseLowInternal5PhysicalVolume = new G4PVPlacement(0,
 G4ThreeVector(x_cylinder_internal_5,y_cylinder_internal_5,-cylinder_height+base_height_internal),
 m_pPipeBaseInternal5LogicalVolume, "Pipe_Flange_Base_internal_5",
 m_pPipeCylinderAirLogicalVolume, false, 0);*/

  m_pPipeCylinderAirPhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(0., 0., 0.), m_pPipeCylinderAirLogicalVolume,
      "Pipe_Air_cylinder_external_vacuum", m_pPipeCylinderSSLogicalVolume,
      false, 0);
  m_pPipeCylinderSSPhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(x_cylinder_external, y_cylinder_external, z_cylinder),
      m_pPipeCylinderSSLogicalVolume, "Pipe_SS_cylinder_external_SS",
      m_pWaterLogicalVolume, false, 0);

  m_pPipeCylinderInternalLowSSPhysicalVolume_1 = new G4PVPlacement(
      0, G4ThreeVector(x_cylinder_internal_1, y_cylinder_internal_1, 0.),
      m_pPipeCylinderInternalLowSSLogicalVolume_1,
      "Pipe_SS_cylinder_low_internal_SS_1", m_pPipeCylinderLowAirLogicalVolume,
      false, 0);
  m_pPipeCylinderInternalLowSSPhysicalVolume_2 = new G4PVPlacement(
      0, G4ThreeVector(x_cylinder_internal_2, y_cylinder_internal_2, 0.),
      m_pPipeCylinderInternalLowSSLogicalVolume_2,
      "Pipe_SS_cylinder_low_internal_SS_2", m_pPipeCylinderLowAirLogicalVolume,
      false, 0);
  m_pPipeCylinderInternalLowSSPhysicalVolume_3 = new G4PVPlacement(
      0, G4ThreeVector(x_cylinder_internal_3, y_cylinder_internal_3, 0.),
      m_pPipeCylinderInternalLowSSLogicalVolume_3,
      "Pipe_SS_cylinder_low_internal_SS_3", m_pPipeCylinderLowAirLogicalVolume,
      false, 0);
  m_pPipeCylinderInternalLowSSPhysicalVolume_4 = new G4PVPlacement(
      0, G4ThreeVector(x_cylinder_internal_4, y_cylinder_internal_4, 0.),
      m_pPipeCylinderInternalLowSSLogicalVolume_4,
      "Pipe_SS_cylinder_low_internal_SS_4", m_pPipeCylinderLowAirLogicalVolume,
      false, 0);
  m_pPipeCylinderInternalLowSSPhysicalVolume_5 = new G4PVPlacement(
      0, G4ThreeVector(x_cylinder_internal_5, y_cylinder_internal_5, 0.),
      m_pPipeCylinderInternalLowSSLogicalVolume_5,
      "Pipe_SS_cylinder_low_internal_SS_5", m_pPipeCylinderLowAirLogicalVolume,
      false, 0);
  m_pPipeCylinderInternalLowSSPhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(0., 0., 0.), m_pPipeCylinderInternalLowSSLogicalVolume,
      "Pipe_SS_cylinder_low_internal_SS", m_pPipeCylinderLowAirLogicalVolume,
      false, 0);

  m_pPipeCylinderLowAirPhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(0., 0., 0.), m_pPipeCylinderLowAirLogicalVolume,
      "Pipe_Air_cylinder_low_external_vacuum",
      m_pPipeCylinderLowSSLogicalVolume, false, 0);
  m_pPipeCylinderLowSSPhysicalVolume = new G4PVPlacement(
      0,
      G4ThreeVector(x_cylinder_external, y_cylinder_external, z_cylinder_low),
      m_pPipeCylinderLowSSLogicalVolume, "Pipe_SS_cylinder_low_external_SS",
      m_pWaterLogicalVolume, false, 0);

  m_pPipeCylinderSmallStainSSPhysicalVolume1 = new G4PVPlacement(
      0,
      G4ThreeVector(x_cylinder_external, y_cylinder_external,
                    z_cylinder + cylinder_height - height_small_stain),
      m_pPipeCylinderSmallStainSSLogicalVolume,
      "Pipe_SS_cylinder_Small_stain_1", m_pWaterLogicalVolume, false, 0);
  m_pPipeCylinderSmallStainSSPhysicalVolume3 = new G4PVPlacement(
      Rot_cyl,
      G4ThreeVector(
          x_cylinder_tilted_long,
          y_torus - height_small_stain * cos(90. * deg - torus_angle) +
              torus_swept_radius * sin(90. * deg - torus_angle),
          z_torus + torus_swept_radius * cos(90. * deg - torus_angle) +
              height_small_stain * sin(90. * deg - torus_angle)),
      m_pPipeCylinderSmallStainSSLogicalVolume,
      "Pipe_SS_cylinder_Small_stain_3", m_pWaterLogicalVolume, false, 0);
  // m_pPipeCylinderSmallStainSSPhysicalVolume2 = new
  // G4PVPlacement(Rot_small_stain, G4ThreeVector(x_cylinder_tilted_long,
  // y_torus-height_small_stain*cos(45.*deg)+torus_swept_radius*sin(45.*deg),
  // z_torus+torus_swept_radius*cos(45.*deg)+height_small_stain*sin(45.*deg)),
  // m_pPipeCylinderSmallStainSSLogicalVolume, "Pipe_cylinder_Small_stain_2",
  // m_pWaterLogicalVolume, false, 0);

  m_pPipeTorusInternalSSPhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(0., 0., 0.), m_pPipeTorusInternalSSLogicalVolume,
      "Pipe_SS_Torus_internal_SS", m_pPipeTorusAirLogicalVolume, false, 0);
  m_pPipeTorusInternalSSPhysicalVolume_1 = new G4PVPlacement(
      0, G4ThreeVector(0., 0., x_torus_internal_1),
      m_pPipeTorusInternalSSLogicalVolume_1, "Pipe_SS_Torus_internal_SS_1",
      m_pPipeTorusAirLogicalVolume, false, 0);
  m_pPipeTorusInternalSSPhysicalVolume_2 = new G4PVPlacement(
      0, G4ThreeVector(0., 0., x_torus_internal_2),
      m_pPipeTorusInternalSSLogicalVolume_2, "Pipe_SS_Torus_internal_SS_2",
      m_pPipeTorusAirLogicalVolume, false, 0);
  m_pPipeTorusInternalSSPhysicalVolume_3 = new G4PVPlacement(
      0, G4ThreeVector(y_torus_internal_3, 0., x_torus_internal_3),
      m_pPipeTorusInternalSSLogicalVolume_3, "Pipe_SS_Torus_internal_SS_3",
      m_pPipeTorusAirLogicalVolume, false, 0);
  m_pPipeTorusInternalSSPhysicalVolume_4 = new G4PVPlacement(
      0, G4ThreeVector(y_torus_internal_4, 0., x_torus_internal_4),
      m_pPipeTorusInternalSSLogicalVolume_4, "Pipe_SS_Torus_internal_SS_4",
      m_pPipeTorusAirLogicalVolume, false, 0);
  m_pPipeTorusInternalSSPhysicalVolume_5 = new G4PVPlacement(
      0, G4ThreeVector(y_torus_internal_5, 0, x_torus_internal_5),
      m_pPipeTorusInternalSSLogicalVolume_5, "Pipe_SS_Torus_internal_SS_5",
      m_pPipeTorusAirLogicalVolume, false, 0);

  m_pPipeTorusAirPhysicalVolume = new G4PVPlacement(
      Rot_torus, G4ThreeVector(x_torus, y_torus, z_torus),
      m_pPipeTorusAirLogicalVolume, "Pipe_Air_Torus_external_vacuum",
      m_pWaterLogicalVolume, false, 0);

  m_pPipeTorusSSPhysicalVolume = new G4PVPlacement(
      Rot_torus, G4ThreeVector(x_torus, y_torus, z_torus),
      m_pPipeTorusSSLogicalVolume, "Pipe_SS_Torus_external_SS",
      m_pWaterLogicalVolume, false, 0);

  m_pPipeTolonPhysicalVolume2 =
      new G4PVPlacement(0, G4ThreeVector(0., 0., -2200. + 235.),
                        m_pPipeTolonLogicalVolume, "Pipe_Torlon_external_SS_2",
                        m_pPipeCylinderTiltedLongAirLogicalVolume, false, 0);
  m_pPipeTolonPhysicalVolume3 =
      new G4PVPlacement(0, G4ThreeVector(0., 0., -2200. + 1414.),
                        m_pPipeTolonLogicalVolume, "Pipe_Torlon_external_SS_3",
                        m_pPipeCylinderTiltedLongAirLogicalVolume, false, 0);
  m_pPipeTolonPhysicalVolume4 =
      new G4PVPlacement(0, G4ThreeVector(0., 0., -2200. + 2615.),
                        m_pPipeTolonLogicalVolume, "Pipe_Torlon_external_SS_4",
                        m_pPipeCylinderTiltedLongAirLogicalVolume, false, 0);
  m_pPipeTolonPhysicalVolume5 =
      new G4PVPlacement(0, G4ThreeVector(0., 0., -2200. + 3820.),
                        m_pPipeTolonLogicalVolume, "Pipe_Torlon_external_SS_5",
                        m_pPipeCylinderTiltedLongAirLogicalVolume, false, 0);

  m_pPipeCylinderTiltedLongInternalSSPhysicalVolume =
      new G4PVPlacement(0, G4ThreeVector(0., 0., 0.),
                        m_pPipeCylinderTiltedLongInternalSSLogicalVolume,
                        "Pipe_SS_cylinder_tilted_long_internal_SS",
                        m_pPipeCylinderTiltedLongAirLogicalVolume, false, 0);
  m_pPipeCylinderTiltedLongInternalSSPhysicalVolume_1 =
      new G4PVPlacement(0, G4ThreeVector(x_offset_internal_1, 0., 0.),
                        m_pPipeCylinderTiltedLongInternalSSLogicalVolume_1,
                        "Pipe_SS_cylinder_tilted_long_internal_SS_1",
                        m_pPipeCylinderTiltedLongAirLogicalVolume, false, 0);
  m_pPipeCylinderTiltedLongInternalSSPhysicalVolume_2 =
      new G4PVPlacement(0, G4ThreeVector(x_offset_internal_2, 0., 0.),
                        m_pPipeCylinderTiltedLongInternalSSLogicalVolume_2,
                        "Pipe_SS_cylinder_tilted_long_internal_SS_2",
                        m_pPipeCylinderTiltedLongAirLogicalVolume, false, 0);
  m_pPipeCylinderTiltedLongInternalSSPhysicalVolume_3 = new G4PVPlacement(
      0,
      G4ThreeVector(x_offset_internal_3, y_cylinder_tilted_long_internal_3,
                    z_cylinder_tilted_long_internal_3),
      m_pPipeCylinderTiltedLongInternalSSLogicalVolume_3,
      "Pipe_SS_cylinder_tilted_long_internal_SS_3",
      m_pPipeCylinderTiltedLongAirLogicalVolume, false, 0);
  m_pPipeCylinderTiltedLongInternalSSPhysicalVolume_4 = new G4PVPlacement(
      0, G4ThreeVector(x_offset_internal_4, y_offset_internal_4, 0.),
      m_pPipeCylinderTiltedLongInternalSSLogicalVolume_4,
      "Pipe_SS_cylinder_tilted_long_internal_SS_4",
      m_pPipeCylinderTiltedLongAirLogicalVolume, false, 0);
  m_pPipeCylinderTiltedLongInternalSSPhysicalVolume_5 = new G4PVPlacement(
      0, G4ThreeVector(x_offset_internal_5, y_offset_internal_5, 0.),
      m_pPipeCylinderTiltedLongInternalSSLogicalVolume_5,
      "Pipe_SS_cylinder_tilted_long_internal_SS_5",
      m_pPipeCylinderTiltedLongAirLogicalVolume, false, 0);

  m_pPipeCylinderTiltedLongAirPhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(0., 0., 0.), m_pPipeCylinderTiltedLongAirLogicalVolume,
      "Pipe_Air_cylinder_tilted_external_long_vacuum",
      m_pPipeCylinderTiltedLongSSLogicalVolume, false, 0);
  m_pPipeCylinderTiltedLongSSPhysicalVolume = new G4PVPlacement(
      Rot_cyl,
      G4ThreeVector(x_cylinder_tilted_long, y_cylinder_tilted_long,
                    z_cylinder_tilted_long),
      m_pPipeCylinderTiltedLongSSLogicalVolume,
      "Pipe_SS_cylinder_Tilted_long_external_SS", m_pWaterLogicalVolume, false,
      0);

  m_pPipeBaseHalfPhysicalVolume = new G4PVPlacement(
      Rot_cyl1, G4ThreeVector(x_base_tilt, y_base_tilt, z_base_tilt),
      m_pPipeBaseLogicalVolume, "Pipe_SS_Flange_cylinder",
      m_pWaterLogicalVolume, false, 0);

  // small pipe

  m_pPipeBaseSmallLowPhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(x_base_small, y_base_small, z_base_small),
      m_pPipeBaseSmallLogicalVolume, "Pipe_SS_Small_Flange_Base",
      m_pWaterLogicalVolume, false, 0);
  m_pPipeCylinderAirSmallPhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(0., 0., 0.), m_pPipeCylinderAirSmallLogicalVolume,
      "Pipe_Air_cylinder_external_small_vacuum",
      m_pPipeCylinderSmallSSLogicalVolume, false, 0);
  m_pPipeCylinderSmallSSPhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(x_base_small, y_base_small, z_cylinder_small),
      m_pPipeCylinderSmallSSLogicalVolume, "Pipe_SS_cylinder_external_small_SS",
      m_pWaterLogicalVolume, false, 0);
  m_pPipeTorusAirSmallPhysicalVolume = new G4PVPlacement(
      Rot_torus, G4ThreeVector(x_base_small, y_torus_small, z_torus_small),
      m_pPipeTorusAirSmallLogicalVolume, "Pipe_Air_Torus_external_small_vacuum",
      m_pWaterLogicalVolume, false, 0);

  m_pPipeTorusSmallSSPhysicalVolume = new G4PVPlacement(
      Rot_torus, G4ThreeVector(x_base_small, y_torus_small, z_torus_small),
      m_pPipeTorusSmallSSLogicalVolume, "Pipe_SS_Torus_external_small_SS",
      m_pWaterLogicalVolume, false, 0);

  m_pPipeCylinderTiltedLongAirSmallPhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(0., 0., 0.),
      m_pPipeCylinderTiltedLongAirSmallLogicalVolume,
      "Pipe_Air_cylinder_tilted_external_long_small_vacuum",
      m_pPipeCylinderTiltedLongSmallSSLogicalVolume, false, 0);
  m_pPipeCylinderTiltedLongSmallSSPhysicalVolume = new G4PVPlacement(
      Rot_cyl,
      G4ThreeVector(x_base_small, y_cylinder_tilted_long_small,
                    z_cylinder_tilted_long_small),
      m_pPipeCylinderTiltedLongSmallSSLogicalVolume,
      "Pipe_SS_cylinder_Tilted_long_external_small_SS", m_pWaterLogicalVolume,
      false, 0);
}

G4UnionSolid *Xenon1tDetectorConstruction::ConstructVessel(
    G4double D, G4double dLength, G4double R0top, G4double R1top, G4double R0bot,
    G4double R1bot, G4double TopCor, G4double BotCor,
    G4double dR_Flange, G4double h_Flange, G4double z_Flange,
    G4double dR_Ring1, G4double h_Ring1, G4double z_Ring1,
    G4double dR_Ring2, G4double h_Ring2, G4double z_Ring2,
    G4bool doBottom, G4bool isInerVesselNT) {

  // ConstructVessel: - a la Columbia University - Gordon Tajiri
  //                  - vessel with two toro-spherical heads.
  //
  // Input:
  //        D              = cylinder diameter
  //        dLength        = cylinder length of the vessel
  //        R0top          = top spherical radius
  //        R1top          = top toroid radius
  //        R0bot          = bottom spherical radius
  //        R1bot          = bottom toroid radius
  //        TopCor         = correction for thicker/thinner top head
  //        BotCor         = correction for thicker/thinner bottom head of vessel
  //        dR_Flange      = dR of flange
  //        h_Flange       = height of flange
  //        z_Flange       = z position of flange
  //        dR_Ring1       = dR of bottom stiffening ring
  //        h_Ring1        = height of bottom stiffening ring
  //        z_Ring1        = z position of bottom stiffening ring
  //        dR_Ring2       = dR of top stiffening ring
  //        h_Ring2        = height of top stiffening ring
  //        z_Ring2        = z position of top stiffening ring
  //        doBottom       = make the bottom torospherical head or not
  //        isInerVesselNT = consider non-symmetrical shape of nT IV along Z axis
  //
  // A.P.Colijn 08-03-2012 / colijn@nikhef.nl
  // D. Ramírez 25.10.2018 - Modifications to handle the complex top dome of the nT
  //                         inner vessel.
  //                       - Modifications to handle the stiffening rings
  //                         for inner and outer cryostat.

  G4double dToleranceUnionSolid = 0.001 * mm;

  // radius of vessel
  G4double R_cyl = D / 2;

  // Cylindrical body with flange. Don't compose of 2xG4Tubs, since you will run
  // into geant4 visualization problems/issues/features ....
  G4Polycone *pTube;

  if (dR_Ring2 == 0) {
      G4double ZZ[] = {-dLength / 2,
                       z_Flange - h_Flange / 2 - 0.001 * cm,
                       z_Flange - h_Flange / 2,
                       z_Flange + h_Flange / 2,
                       z_Flange + h_Flange / 2 + 0.001 * cm,
                       dLength / 2};
      G4double RRin[] = {0, 0, 0, 0, 0, 0};
      G4double RRout[] = {R_cyl, R_cyl, R_cyl + dR_Flange, R_cyl + dR_Flange,
                          R_cyl, R_cyl};

      pTube = new G4Polycone("Tube", 0., 2 * M_PI, 6, ZZ, RRin, RRout);
  } else {
      G4double ZZ[] = {-dLength / 2,
                       z_Ring1 - h_Ring1 / 2 - 0.001 * cm,
                       z_Ring1 - h_Ring1 / 2,
                       z_Ring1 + h_Ring1 / 2,
                       z_Ring1 + h_Ring1 / 2 + 0.001 * cm,
                       z_Ring2 - h_Ring2 / 2 - 0.001 * cm,
                       z_Ring2 - h_Ring2 / 2,
                       z_Ring2 + h_Ring2 / 2,
                       z_Ring2 + h_Ring2 / 2 + 0.001 * cm,
                       z_Flange - h_Flange / 2 - 0.001 * cm,
                       z_Flange - h_Flange / 2,
                       z_Flange + h_Flange / 2,
                       z_Flange + h_Flange / 2 + 0.001 * cm,
                       dLength / 2};
      G4double RRin[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
      G4double RRout[] = {R_cyl, R_cyl, R_cyl + dR_Ring1, R_cyl + dR_Ring1,
                          R_cyl, R_cyl, R_cyl + dR_Ring2, R_cyl + dR_Ring2,
                          R_cyl, R_cyl, R_cyl + dR_Flange, R_cyl + dR_Flange,
                          R_cyl, R_cyl};

      pTube = new G4Polycone("Tube", 0., 2 * M_PI, 14, ZZ, RRin, RRout);
  }

  // Help variables to calculate coordinates of cone and torus

  // For the bottom
  G4double rc0 = R_cyl - R1bot - BotCor;
  G4double dR0 = R0bot - R1bot;

  G4double dTheta_bot = asin(rc0 / dR0);
  G4double dZ_bot = sqrt(dR0 * dR0 - rc0 * rc0);

  G4Sphere *pBottom1 = new G4Sphere("Bottom1", 0.6 * R0bot, R0bot, 0., 2 * M_PI,
                                    M_PI - dTheta_bot, dTheta_bot);
  G4Torus *pBottom2 = new G4Torus("Bottom2", 0. * cm, R1bot, rc0, 0, 2 * M_PI);

  // Continue with the top head of the vessel
  rc0 = R_cyl - R1top - TopCor;
  dR0 = R0top - R1top;

  G4double dTheta_top = asin(rc0 / dR0);
  G4double dZ_top = sqrt(dR0 * dR0 - rc0 * rc0);

  if (m_iVerbosityLevel >= 1)
    G4cout << "  - Top   : rc0=" << rc0 << " dR0=" << dR0 << " dTheta="
           << dTheta_top << " dZ=" << dZ_top;

  G4Sphere *pTop1 =
      new G4Sphere("Top1", 0.6 * R0top, R0top, 0., 2 * M_PI, 0., dTheta_top);
  G4Torus *pTop2 = new G4Torus("Top2", 0. * cm, R1top, rc0, 0, 2 * M_PI);

  // Make one solid of the vessel components
  G4UnionSolid *pVessel;

  G4double zPos = dLength / 2;
  if (m_iVerbosityLevel >= 1)
    G4cout << " zPos_toroid=" << zPos;

  // DR 20190301 - nT inner vessel torospherical shape on top is
  //               slightly higher (Z) than the bottom one.
  if (isInerVesselNT) {
    pVessel = new G4UnionSolid("UnionSolid", pTube, pTop2, 0,
                               G4ThreeVector(dToleranceUnionSolid, 0,
                                             zPos + 0.58 * mm));
    zPos = dLength / 2 - dZ_top + 0.58 * mm;
  }
  else {
    pVessel = new G4UnionSolid("UnionSolid", pTube, pTop2, 0,
                               G4ThreeVector(dToleranceUnionSolid, 0, zPos));
    zPos = dLength / 2 - dZ_top;
  }

  if (m_iVerbosityLevel >= 1)
    G4cout << " zPos_sphere=" << zPos << G4endl;
  pVessel = new G4UnionSolid("UnionSolid", pVessel, pTop1, 0,
                             G4ThreeVector(dToleranceUnionSolid, 0, zPos));

  if (doBottom) {
    if (m_iVerbosityLevel >= 1)
      G4cout << "  - Bottom: rc0=" << rc0 << " dR0=" << dR0 << " dTheta="
             << dTheta_bot << " dZ=" << dZ_bot;

    zPos = -dLength / 2;
    if (m_iVerbosityLevel >= 1)
      G4cout << " zPos_toroid=" << zPos;
    pVessel = new G4UnionSolid("UnionSolid", pVessel, pBottom2, 0,
                               G4ThreeVector(dToleranceUnionSolid, 0, zPos));

    zPos = -dLength / 2 + dZ_bot;
    if (m_iVerbosityLevel >= 1)
      G4cout << " zPos_sphere=" << zPos << G4endl;
    pVessel = new G4UnionSolid("UnionSolid", pVessel, pBottom1, 0,
                               G4ThreeVector(dToleranceUnionSolid, 0, zPos));
  }

  return pVessel;
}

//=== Liquid Scintillator nVeto === // DR 20161115
// (Number of side vessels and thickness of tom/bottom and side vessels
// configurable from macros)
void Xenon1tDetectorConstruction::ConstructLScintVessel() {
  // G4double zPos = GetGeometryParameter("OuterCryostatOffsetZ");
  //  G4double zPos = GetGeometryParameter("z_nVetoOffset");

  // // DR 20170116 - Creation of two water volumes above and below (touching)
  // the LScint volume, in order to define the reflector foil in the interface.
  // G4Material *WaterInterfaceMaterial = G4Material::GetMaterial("Water");

  // G4double r_up   = GetGeometryParameter("VetoPipesFeedthroughRadius");
  // G4double r_down = GetGeometryParameter("VetoChainFeedthroughRadius");
  // G4double R      = GetGeometryParameter("LScintVetoCylinderRadius");
  // G4double L      = GetGeometryParameter("LScintVetoCylinderHeight");
  // G4double L_up   = L + 1.*cm + zPos;
  // G4double L_down = -L - 1.*cm + zPos;

  // G4Tubs *pWater_interface_up = new G4Tubs("Water_interface_up", r_up, R,
  // 1.*cm, 0.*deg, 360.*deg);
  // m_pWater_interface_upLogicalVolume = new
  // G4LogicalVolume(pWater_interface_up, WaterInterfaceMaterial,
  // "Water_interface_up", 0, 0, 0);
  // m_pWater_interface_upPhysicalVolume = new G4PVPlacement(0,
  // G4ThreeVector(0.*mm, 0.*mm, L_up),
  // 						    m_pWater_interface_upLogicalVolume,
  // "Water_interface_up", m_pWaterLogicalVolume, false, 0);
  // G4Tubs *pWater_interface_down = new G4Tubs("Water_interface_down", r_down,
  // R, 1.*cm, 0.*deg, 360.*deg);
  // m_pWater_interface_downLogicalVolume = new
  // G4LogicalVolume(pWater_interface_down, WaterInterfaceMaterial,
  // "Water_interface_up", 0, 0, 0);
  // m_pWater_interface_downPhysicalVolume = new G4PVPlacement(0,
  // G4ThreeVector(0.*mm, 0.*mm, L_down),
  // 						    m_pWater_interface_downLogicalVolume,
  // "Water_interface_down", m_pWaterLogicalVolume, false, 0);

  // define the acrylic vessel, foam and SS platform
  G4double AcrylicThickness = GetGeometryParameter("LScintVesselThickness");
  G4double Rin = GetGeometryParameter("LScintVesselInnerRadius");
  G4double Rout = GetGeometryParameter("LScintVesselOuterRadius");
  G4double H = GetGeometryParameter("LScintVesselHeight");
  G4double zOffset =
      GetGeometryParameter("TankOffsetZ") + 0.5 * H - 559.5 * mm -
      103.5 * mm;  //- 154*mm;  //center in Z of the Acrylic Vessel
  if (m_iVerbosityLevel >= 1)
    G4cout << "Acrylic dimensions: " << Rin << " " << Rout << " " << H
           << G4endl;

  // Define the cylindrical SS platform
  G4Material *SS316Ti = G4Material::GetMaterial("SS316Ti");
  G4double nVetoFloorThickness = 8 * mm;
  G4Tubs *pnVetoFloorTube =
      new G4Tubs("nVetoFloorTube", Rin, Rout, nVetoFloorThickness / 2.,
                 0. * deg, 360. * deg);
  m_pnVetoFloorLogicalVolume =
      new G4LogicalVolume(pnVetoFloorTube, SS316Ti, "nVetoFloor", 0, 0, 0);
  m_pnVetoFloorPhysicalVolume = new G4PVPlacement(
      0,
      G4ThreeVector(0. * mm, 0. * mm,
                    zOffset - 0.5 * H - 0.5 * nVetoFloorThickness),
      m_pnVetoFloorLogicalVolume, "nVetoFloor", m_pWaterLogicalVolume, false,
      0);

  // Define the foam inside the acrylic vessels (the cryostat will be a daughter
  // of the foam)
  // G4Material *WaterDisplacerMaterial = G4Material::GetMaterial("Water"); //
  // Water, Foam, Air (G4_AIR), Vacuum...
  G4Material *WaterDisplacerMaterial =
      G4Material::GetMaterial("Foam");  // Water, Foam, Air (G4_AIR), Vacuum...
  // G4Material *WaterDisplacerMaterial = G4Material::GetMaterial("Graphite");
  // // Water, Foam, Air (G4_AIR), Vacuum...

  G4Tubs *pWaterDisplacerTube =
      new G4Tubs("WaterDisplacerTube", 0., Rin, H / 2., 0. * deg, 360. * deg);
  m_pWaterDisplacerLogicalVolume = new G4LogicalVolume(
      pWaterDisplacerTube, WaterDisplacerMaterial, "WaterDisplacer", 0, 0, 0);
  m_pWaterDisplacerPhysicalVolume =
      new G4PVPlacement(0, G4ThreeVector(0. * mm, 0. * mm, zOffset),
                        m_pWaterDisplacerLogicalVolume, "WaterDisplacer",
                        m_pWaterLogicalVolume, false, 0);

  // Define the cylindrical acrylic volume
  G4Material *VetoAcrylicMaterial = G4Material::GetMaterial("Acrylic");
  G4Tubs *pAcrylicVesselTube =
      new G4Tubs("AcrylicVesselTube", Rin, Rout, H / 2., 0. * deg, 360. * deg);
  m_pVetoAcrylicLogicalVolume = new G4LogicalVolume(
      pAcrylicVesselTube, VetoAcrylicMaterial, "AcrylicVessel", 0, 0, 0);
  m_pVetoAcrylicPhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(0. * mm, 0. * mm, zOffset), m_pVetoAcrylicLogicalVolume,
      "AcrylicVessel", m_pWaterLogicalVolume, false, 0);

  // DR 20160916 - Input of veto LScint mixture from the macros.
  G4Material *LScintVesselMaterial;
  if (pLScintVesselMaterial) {
    if (pLScintVesselMaterial == "Water" || pLScintVesselMaterial == "LScint" ||
        pLScintVesselMaterial == "Gd_LScint" ||
        pLScintVesselMaterial == "B_LScint" ||
        pLScintVesselMaterial == "Gd_LScint_0_2" ||
        pLScintVesselMaterial == "Gd_LScint_0_4" ||
        pLScintVesselMaterial == "Gd_LScint_0_8") {
      LScintVesselMaterial = G4Material::GetMaterial(pLScintVesselMaterial);
    }

    else {
      G4Exception("XenonDetectorConstruction::ConstructLScintVessel()",
                  "DetectorConstruction", FatalException,
                  "Not allowed Material for the LScint Vessel");
    }
  }

  // now the LScint inside the Acrylic (shifted by 'AcrylicThickness')
  G4double rin = Rin + AcrylicThickness;
  G4double rout = rin + GetGeometryParameter("LScintVesselThicknessSides");
  G4double h = 1267. * mm - 2 * AcrylicThickness;  // 1375.*mm
  G4double sAngle = 90. * deg;                     // 90.*deg

  //// BOTTOM RING (z = -H/2. + AcrylicThickness) ////
  G4double z_0 = -H / 2. + AcrylicThickness + h / 2.;
  if (m_iVerbosityLevel >= 1)
    G4cout << "LScint dimensions: " << rin << " " << rout << " " << h << G4endl;

  // slice 0 (s00)
  G4Tubs *pLScintVessel_s00 =
      new G4Tubs("LScintVessel 00", rin, rout, h / 2., 0. * deg, sAngle);
  m_pLScintVesselLogicalVolume_s00 = new G4LogicalVolume(
      pLScintVessel_s00, LScintVesselMaterial, "LScintVessel_00", 0, 0, 0);
  m_pLScintVesselPhysicalVolume_s00 = new G4PVPlacement(
      0, G4ThreeVector(0, 0, z_0), m_pLScintVesselLogicalVolume_s00,
      "LScintVessel", m_pVetoAcrylicLogicalVolume, false, 0);

  // slice 1 (s01)
  G4Tubs *pLScintVessel_s01 =
      new G4Tubs("LScintVessel 01", rin, rout, h / 2., 90. * deg, sAngle);
  m_pLScintVesselLogicalVolume_s01 = new G4LogicalVolume(
      pLScintVessel_s01, LScintVesselMaterial, "LScintVessel_01", 0, 0, 0);
  m_pLScintVesselPhysicalVolume_s01 = new G4PVPlacement(
      0, G4ThreeVector(0, 0, z_0), m_pLScintVesselLogicalVolume_s01,
      "LScintVessel", m_pVetoAcrylicLogicalVolume, false, 0);

  // slice 2 (s02)
  G4Tubs *pLScintVessel_s02 =
      new G4Tubs("LScintVessel 02", rin, rout, h / 2., 180. * deg, sAngle);
  m_pLScintVesselLogicalVolume_s02 = new G4LogicalVolume(
      pLScintVessel_s02, LScintVesselMaterial, "LScintVessel_02", 0, 0, 0);
  m_pLScintVesselPhysicalVolume_s02 = new G4PVPlacement(
      0, G4ThreeVector(0, 0, z_0), m_pLScintVesselLogicalVolume_s02,
      "LScintVessel", m_pVetoAcrylicLogicalVolume, false, 0);

  // slice 3 (s03)
  G4Tubs *pLScintVessel_s03 =
      new G4Tubs("LScintVessel 03", rin, rout, h / 2., 270. * deg, sAngle);
  m_pLScintVesselLogicalVolume_s03 = new G4LogicalVolume(
      pLScintVessel_s03, LScintVesselMaterial, "LScintVessel_03", 0, 0, 0);
  m_pLScintVesselPhysicalVolume_s03 = new G4PVPlacement(
      0, G4ThreeVector(0, 0, z_0), m_pLScintVesselLogicalVolume_s03,
      "LScintVessel", m_pVetoAcrylicLogicalVolume, false, 0);

  //// MIDDLE RING (z = -H/2. + 3*AcrylicThickness + 1.5*h) ////
  h = 1267. * mm - 2 * AcrylicThickness;  ////1375.*mm
  z_0 = -H / 2. + 3 * AcrylicThickness +
        1.5 * h;  // change it if the bottom and middle layers have different h
  if (m_iVerbosityLevel >= 1)
    G4cout << "Middle ring dimensions: " << rin << " " << rout << " " << h
           << G4endl;

  // slice 0 (s10)
  G4Tubs *pLScintVessel_s10 =
      new G4Tubs("LScintVessel  10", rin, rout, h / 2., 0. * deg, sAngle);
  m_pLScintVesselLogicalVolume_s10 = new G4LogicalVolume(
      pLScintVessel_s10, LScintVesselMaterial, "LScintVessel_10", 0, 0, 0);
  m_pLScintVesselPhysicalVolume_s10 = new G4PVPlacement(
      0, G4ThreeVector(0, 0, z_0), m_pLScintVesselLogicalVolume_s10,
      "LScintVessel", m_pVetoAcrylicLogicalVolume, false, 0);

  // slice 1 (s11)
  G4Tubs *pLScintVessel_s11 =
      new G4Tubs("LScintVessel 11", rin, rout, h / 2., 90. * deg, sAngle);
  m_pLScintVesselLogicalVolume_s11 = new G4LogicalVolume(
      pLScintVessel_s11, LScintVesselMaterial, "LScintVessel_11", 0, 0, 0);
  m_pLScintVesselPhysicalVolume_s11 = new G4PVPlacement(
      0, G4ThreeVector(0, 0, z_0), m_pLScintVesselLogicalVolume_s11,
      "LScintVessel", m_pVetoAcrylicLogicalVolume, false, 0);

  // slice 2 (s12)
  G4Tubs *pLScintVessel_s12 =
      new G4Tubs("LScintVessel 12", rin, rout, h / 2., 180. * deg, sAngle);
  m_pLScintVesselLogicalVolume_s12 = new G4LogicalVolume(
      pLScintVessel_s12, LScintVesselMaterial, "LScintVessel_12", 0, 0, 0);
  m_pLScintVesselPhysicalVolume_s12 = new G4PVPlacement(
      0, G4ThreeVector(0, 0, z_0), m_pLScintVesselLogicalVolume_s12,
      "LScintVessel", m_pVetoAcrylicLogicalVolume, false, 0);

  // slice 3 (s13)
  G4Tubs *pLScintVessel_s13 =
      new G4Tubs("LScintVessel 13", rin, rout, h / 2., 270. * deg, sAngle);
  m_pLScintVesselLogicalVolume_s13 = new G4LogicalVolume(
      pLScintVessel_s13, LScintVesselMaterial, "LScintVessel_13", 0, 0, 0);
  m_pLScintVesselPhysicalVolume_s13 = new G4PVPlacement(
      0, G4ThreeVector(0, 0, z_0), m_pLScintVesselLogicalVolume_s13,
      "LScintVessel", m_pVetoAcrylicLogicalVolume, false, 0);

  //// TOP RING (z = H/2. - AcrylicThickness - h/2) ////
  h = 1267. * mm - 2 * AcrylicThickness;  // 1050.*mm
  z_0 = H / 2. - AcrylicThickness - h / 2;
  if (m_iVerbosityLevel >= 1)
    G4cout << "Top ring dimensions: " << rin << " " << rout << " " << h
           << G4endl;

  // slice 0 (s20)
  G4Tubs *pLScintVessel_s20 =
      new G4Tubs("LScintVessel 20", rin, rout, h / 2., 0. * deg, sAngle);
  m_pLScintVesselLogicalVolume_s20 = new G4LogicalVolume(
      pLScintVessel_s20, LScintVesselMaterial, "LScintVessel_20", 0, 0, 0);
  m_pLScintVesselPhysicalVolume_s20 = new G4PVPlacement(
      0, G4ThreeVector(0, 0, z_0), m_pLScintVesselLogicalVolume_s20,
      "LScintVessel", m_pVetoAcrylicLogicalVolume, false, 0);

  // slice 1 (s21)
  G4Tubs *pLScintVessel_s21 =
      new G4Tubs("LScintVessel 21", rin, rout, h / 2., 90. * deg, sAngle);
  m_pLScintVesselLogicalVolume_s21 = new G4LogicalVolume(
      pLScintVessel_s21, LScintVesselMaterial, "LScintVessel_21", 0, 0, 0);
  m_pLScintVesselPhysicalVolume_s21 = new G4PVPlacement(
      0, G4ThreeVector(0, 0, z_0), m_pLScintVesselLogicalVolume_s21,
      "LScintVessel", m_pVetoAcrylicLogicalVolume, false, 0);

  // slice 2 (s22)
  G4Tubs *pLScintVessel_s22 =
      new G4Tubs("LScintVessel 22", rin, rout, h / 2., 180. * deg, sAngle);
  m_pLScintVesselLogicalVolume_s22 = new G4LogicalVolume(
      pLScintVessel_s22, LScintVesselMaterial, "LScintVessel_22", 0, 0, 0);
  m_pLScintVesselPhysicalVolume_s22 = new G4PVPlacement(
      0, G4ThreeVector(0, 0, z_0), m_pLScintVesselLogicalVolume_s22,
      "LScintVessel", m_pVetoAcrylicLogicalVolume, false, 0);

  // slice 3 (s23)
  G4Tubs *pLScintVessel_s23 =
      new G4Tubs("LScintVessel 23", rin, rout, h / 2., 270. * deg, sAngle);
  m_pLScintVesselLogicalVolume_s23 = new G4LogicalVolume(
      pLScintVessel_s23, LScintVesselMaterial, "LScintVessel_23", 0, 0, 0);
  m_pLScintVesselPhysicalVolume_s23 = new G4PVPlacement(
      0, G4ThreeVector(0, 0, z_0), m_pLScintVesselLogicalVolume_s23,
      "LScintVessel", m_pVetoAcrylicLogicalVolume, false, 0);

  //----

  // // Define the cylindrical volume
  // G4Tubs *pLScintVessel = new G4Tubs("LScintVessel", 0, R, L, 0.*deg,
  // 360.*deg);

  // // make the logical volume
  // m_pLScintVesselLogicalVolume = new G4LogicalVolume(pLScintVessel,
  // LScintVesselMaterial, "LScintVessel", 0, 0, 0);

  // m_pLScintVesselPhysicalVolume = new G4PVPlacement(0, G4ThreeVector(0.*mm,
  // 0.*mm, zPos),
  // m_pLScintVesselLogicalVolume, "LScintVessel", m_pWaterLogicalVolume,
  // false, 0);

  //------------------------------ LScint vessel sensitivity
  //------------------------------ DR 20160906
  G4SDManager *pSDManager = G4SDManager::GetSDMpointer();
  Xenon1tLScintSensitiveDetector *pLScintSD =
      new Xenon1tLScintSensitiveDetector("Xenon1t/LScintSD");
  pSDManager->AddNewDetector(pLScintSD);

  m_pLScintVesselLogicalVolume_s00->SetSensitiveDetector(pLScintSD);
  m_pLScintVesselLogicalVolume_s01->SetSensitiveDetector(pLScintSD);
  m_pLScintVesselLogicalVolume_s02->SetSensitiveDetector(pLScintSD);
  m_pLScintVesselLogicalVolume_s03->SetSensitiveDetector(pLScintSD);

  m_pLScintVesselLogicalVolume_s10->SetSensitiveDetector(pLScintSD);
  m_pLScintVesselLogicalVolume_s11->SetSensitiveDetector(pLScintSD);
  m_pLScintVesselLogicalVolume_s12->SetSensitiveDetector(pLScintSD);
  m_pLScintVesselLogicalVolume_s13->SetSensitiveDetector(pLScintSD);

  m_pLScintVesselLogicalVolume_s20->SetSensitiveDetector(pLScintSD);
  m_pLScintVesselLogicalVolume_s21->SetSensitiveDetector(pLScintSD);
  m_pLScintVesselLogicalVolume_s22->SetSensitiveDetector(pLScintSD);
  m_pLScintVesselLogicalVolume_s23->SetSensitiveDetector(pLScintSD);

  // redefine mother volume for the cryostat
  m_pMotherLogicalVolume = m_pWaterDisplacerLogicalVolume;
}

void Xenon1tDetectorConstruction::ConstructVetoAcrylic() {
  G4Material *VetoAcrylicMaterial = G4Material::GetMaterial("Acrylic");

  ////////
  // Side//
  ////////
  G4double r = GetGeometryParameter("WaterDisplacerCylinderRadius");
  G4double R = r + GetGeometryParameter("LScintVesselThicknessSides");
  G4double dLength = GetGeometryParameter("WaterDisplacerCylinderHeight") +
                     GetGeometryParameter("LScintVesselThicknessTopBottom");
  G4double r_cut = r + 2.54 * cm;
  G4double R_cut = R - 2.54 * cm;
  G4double L_cut = dLength - 2.54 * cm;
  // G4double zPos = GetGeometryParameter("OuterCryostatOffsetZ");
  // G4double zPos = GetGeometryParameter("z_nVetoOffset");

  G4Tubs *VetoAcrylicSide_main =
      new G4Tubs("VetoAcrylicSide_main", r, R, dLength, 0. * deg, 360. * deg);
  G4Tubs *VetoAcrylicSide_cut = new G4Tubs("VetoAcrylicSide_cut", r_cut, R_cut,
                                           L_cut, 0. * deg, 360. * deg);
  G4SubtractionSolid *pVetoAcrylicSide = new G4SubtractionSolid(
      "VetoAcrylicSide", VetoAcrylicSide_main, VetoAcrylicSide_cut);

  //////////
  // Wedges//
  //////////
  G4double wedge_side =
      0.5 * (GetGeometryParameter("LScintVesselThicknessSides")) - 2. * cm;
  G4double wedge_height =
      GetGeometryParameter("LScintVetoCylinderHeight") - 2. * cm;
  // In both cases: 2 cm instead of 2.54 cm, to be able to merge the wedges and
  // the side vessels afterwards.
  G4double r_wedge = GetGeometryParameter("WaterDisplacerCylinderRadius") +
                     0.5 * (GetGeometryParameter("LScintVesselThicknessSides"));

  G4Box *pVetoAcrylicWedge =
      new G4Box("AcrylicWedge", 1.27 * cm, wedge_side,
                wedge_height);  // 1.27 cm = half of an inch

  G4RotationMatrix *zRot_four_wedges =
      new G4RotationMatrix;                    // Rotates X and Y axes only
  zRot_four_wedges->rotateZ(M_PI / 2. * rad);  // Rotates 90 degrees

  G4RotationMatrix *zRot_three_wedges = new G4RotationMatrix;
  G4RotationMatrix *zRot_three_wedges_inv = new G4RotationMatrix;
  zRot_three_wedges->rotateZ(2 * M_PI / 3. * rad);       // Rotates 120 degrees
  zRot_three_wedges_inv->rotateZ(-2 * M_PI / 3. * rad);  // Rotates -120 degrees

  /////////
  // Above//
  /////////
  G4double r_above = GetGeometryParameter("VetoPipesFeedthroughRadius");
  G4double R_above = r + 1. * cm;  // For it to overlap with the side vessels
                                   // (to merge both volumes)
  G4double L_above =
      0.5 * (GetGeometryParameter("LScintVesselThicknessTopBottom"));
  G4double r_above_cut = r_above + 2.54 * cm;
  G4double R_above_cut =
      R_above - 2.54 * cm - 1. * cm;  // For it to overlap with the side vessels
                                      // (to merge both volumes)
  G4double L_above_cut = L_above - 2.54 * cm;
  // Height of the center of the top vessel
  G4double zAbove =
      GetGeometryParameter("WaterDisplacerCylinderHeight") +
      0.5 * (GetGeometryParameter("LScintVesselThicknessTopBottom"));

  G4double wedge_above_side =
      0.5 * (GetGeometryParameter("WaterDisplacerCylinderRadius") - r_above) -
      2. * cm;
  G4double wedge_above_height = L_above - 2. * cm;
  // In both cases: 2 cm instead of 2.54 cm, to be able to merge the wedges and
  // the side vessels afterwards.
  G4double r_wedge_above =
      r_above +
      0.5 * (GetGeometryParameter("WaterDisplacerCylinderRadius") - r_above);
  G4Box *pVetoAcrylicWedge_Above =
      new G4Box("AcrylicWedge_Above", 1.27 * cm, wedge_above_side,
                wedge_above_height);  // 1.27 cm = half of an inch

  G4Tubs *VetoAcrylicAbove_main = new G4Tubs(
      "VetoAcrylicAbove_main", r_above, R_above, L_above, 0. * deg, 360. * deg);
  G4Tubs *VetoAcrylicAbove_cut =
      new G4Tubs("VetoAcrylicAbove_cut", r_above_cut, R_above_cut, L_above_cut,
                 0. * deg, 360. * deg);

  G4SubtractionSolid *pVetoAcrylicAbove_step1 = new G4SubtractionSolid(
      "VetoAcrylicAbove_step1", VetoAcrylicAbove_main, VetoAcrylicAbove_cut);

  G4UnionSolid *pVetoAcrylicAbove_step2 = new G4UnionSolid(
      "VetoAcrylicAbove_step2", pVetoAcrylicAbove_step1,
      pVetoAcrylicWedge_Above, 0, G4ThreeVector(0, r_wedge_above, 0));
  G4UnionSolid *pVetoAcrylicAbove = new G4UnionSolid(
      "VetoAcrylicAbove", pVetoAcrylicAbove_step2, pVetoAcrylicWedge_Above, 0,
      G4ThreeVector(0, -r_wedge_above, 0));

  /////////
  // Below//
  /////////
  G4double r_below = GetGeometryParameter("VetoChainFeedthroughRadius");
  G4double R_below = R_above;
  G4double L_below = L_above;
  G4double r_below_cut = r_below + 2.54 * cm;
  G4double R_below_cut = R_above_cut;
  G4double L_below_cut = L_above_cut;
  // Height of the center of the bottom vessel
  G4double zBelow = -zAbove;

  G4Tubs *VetoAcrylicBelow_main = new G4Tubs(
      "VetoAcrylicBelow_main", r_below, R_below, L_below, 0. * deg, 360. * deg);
  G4Tubs *VetoAcrylicBelow_cut =
      new G4Tubs("VetoAcrylicBelow_cut", r_below_cut, R_below_cut, L_below_cut,
                 0. * deg, 360. * deg);
  G4SubtractionSolid *pVetoAcrylicBelow = new G4SubtractionSolid(
      "VetoAcrylicBelow", VetoAcrylicBelow_main, VetoAcrylicBelow_cut);

  ///////////////////
  // Merging volumes//
  ///////////////////
  G4UnionSolid *pVetoAcrylic;

  if (pLScintNumberOfSideVessels == "3") {  // For 3 side vessels

    G4UnionSolid *pVetoAcrylic_step1 =
        new G4UnionSolid("VetoAcrylic_step1", pVetoAcrylicSide,
                         pVetoAcrylicWedge, 0, G4ThreeVector(0., r_wedge, 0.));
    G4UnionSolid *pVetoAcrylic_step2 = new G4UnionSolid(
        "VetoAcrylic_step2", pVetoAcrylic_step1, pVetoAcrylicWedge,
        zRot_three_wedges_inv,
        G4ThreeVector(-r_wedge * cos(M_PI / 6. * rad),
                      -r_wedge * sin(M_PI / 6. * rad), 0. * mm));
    G4UnionSolid *pVetoAcrylic_step3 = new G4UnionSolid(
        "VetoAcrylic_step3", pVetoAcrylic_step2, pVetoAcrylicWedge,
        zRot_three_wedges,
        G4ThreeVector(r_wedge * cos(M_PI / 6. * rad),
                      -r_wedge * sin(M_PI / 6. * rad), 0. * mm));

    if (pConstructLScintTopVessel) {
      G4UnionSolid *pVetoAcrylic_step4 =
          new G4UnionSolid("VetoAcrylic_step4", pVetoAcrylic_step3,
                           pVetoAcrylicAbove, 0, G4ThreeVector(0., 0., zAbove));
      pVetoAcrylic =
          new G4UnionSolid("VetoAcrylic", pVetoAcrylic_step4, pVetoAcrylicBelow,
                           0, G4ThreeVector(0., 0., zBelow));
    } else {
      pVetoAcrylic =
          new G4UnionSolid("VetoAcrylic", pVetoAcrylic_step3, pVetoAcrylicBelow,
                           0, G4ThreeVector(0., 0., zBelow));
    }

  }

  else if (pLScintNumberOfSideVessels == "4") {  // For 4 side vessels

    G4UnionSolid *pVetoAcrylic_step1 =
        new G4UnionSolid("VetoAcrylic_step1", pVetoAcrylicSide,
                         pVetoAcrylicWedge, 0, G4ThreeVector(0., -r_wedge, 0.));
    G4UnionSolid *pVetoAcrylic_step2 =
        new G4UnionSolid("VetoAcrylic_step2", pVetoAcrylic_step1,
                         pVetoAcrylicWedge, 0, G4ThreeVector(0., r_wedge, 0.));
    G4UnionSolid *pVetoAcrylic_step3 = new G4UnionSolid(
        "VetoAcrylic_step3", pVetoAcrylic_step2, pVetoAcrylicWedge,
        zRot_four_wedges, G4ThreeVector(r_wedge, 0., 0.));
    G4UnionSolid *pVetoAcrylic_step4 = new G4UnionSolid(
        "VetoAcrylic_step4", pVetoAcrylic_step3, pVetoAcrylicWedge,
        zRot_four_wedges, G4ThreeVector(-r_wedge, 0., 0.));

    if (pConstructLScintTopVessel) {
      G4UnionSolid *pVetoAcrylic_step5 =
          new G4UnionSolid("VetoAcrylic_step5", pVetoAcrylic_step4,
                           pVetoAcrylicAbove, 0, G4ThreeVector(0., 0., zAbove));
      pVetoAcrylic =
          new G4UnionSolid("VetoAcrylic", pVetoAcrylic_step5, pVetoAcrylicBelow,
                           0, G4ThreeVector(0., 0., zBelow));
    } else {
      pVetoAcrylic =
          new G4UnionSolid("VetoAcrylic", pVetoAcrylic_step4, pVetoAcrylicBelow,
                           0, G4ThreeVector(0., 0., zBelow));
    }
  }

  else {
    G4Exception(
        "XenonDetectorConstruction::ConstructLScintVessel()",
        "DetectorConstruction", FatalException,
        "Invalid number of LScint side vessels (they must be '3' or '4')");
  }

  m_pVetoAcrylicLogicalVolume = new G4LogicalVolume(
      pVetoAcrylic, VetoAcrylicMaterial, "VetoAcrylic", 0, 0, 0);
  m_pVetoAcrylicPhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(0. * mm, 0. * mm, 0. * mm), m_pVetoAcrylicLogicalVolume,
      "VetoAcrylic", m_pMotherLogicalVolume, false, 0);

  //-------------------------------------------------------------------------
  // DR 20170116
  G4OpticalSurface *OptLScintSurface = new G4OpticalSurface("LScintSurface");
  OptLScintSurface->SetType(dielectric_metal);  //(dielectric_LUT)
  OptLScintSurface->SetModel(unified);          //(LUT);
  OptLScintSurface->SetFinish(polished);        // Specular Reflector

  const G4int NUM = 7;

  G4double pp[NUM] = {2.034 * eV, 3.179 * eV, 3.351 * eV, 3.701 * eV,
                      4.065 * eV, 4.509 * eV, 4.959 * eV};
  G4double reflectivity_W[NUM] = {0.95, 0.95, 0.07, 0.05, 0.11, 0.06, 0.19};
  G4MaterialPropertiesTable *SMPT_W = new G4MaterialPropertiesTable();
  SMPT_W->AddProperty("REFLECTIVITY", pp, reflectivity_W, NUM);

  OptLScintSurface->SetMaterialPropertiesTable(SMPT_W);

  new G4LogicalBorderSurface(
      "OuterLScintSurface_up", m_pVetoAcrylicPhysicalVolume,
      m_pWater_interface_upPhysicalVolume, OptLScintSurface);
  new G4LogicalBorderSurface(
      "OuterLScintSurface_down", m_pVetoAcrylicPhysicalVolume,
      m_pWater_interface_downPhysicalVolume, OptLScintSurface);
  //-------------------------------------------------------------------------
}

void Xenon1tDetectorConstruction::ConstructWaterDisplacer() {
  G4Material *WaterDisplacerMaterial = G4Material::GetMaterial(
      "G4_AIR");  // Water, Foam, Air (G4_AIR), Vacuum...

  // Upper feedthrough in the acrylic vessels filled with the "WaterDisplacer"
  // volume
  G4double R_up;
  if (pConstructLScintTopVessel)
    R_up = GetGeometryParameter("VetoPipesFeedthroughRadius");
  else
    R_up = GetGeometryParameter("WaterDisplacerCylinderRadius");
  G4double L_up =
      0.5 * (GetGeometryParameter("LScintVesselThicknessTopBottom")) +
      1. * cm;  // For it to overlap with the merge volume (to merge them)
  G4double zPos_up =
      GetGeometryParameter("WaterDisplacerCylinderHeight") +
      0.5 * (GetGeometryParameter("LScintVesselThicknessTopBottom")) - 1. * cm;
  // - 1 cm for it NOT to ovelap with the LScinVessel volume, after the 1 cm
  // increase from above
  G4Tubs *pWaterDisplacer_up =
      new G4Tubs("WaterDisplacer_up", 0, R_up, L_up, 0. * deg, 360. * deg);

  // Lower feedthrough in the acrylic vessels filled with the "WaterDisplacer"
  // volume
  G4double R_down = GetGeometryParameter("VetoChainFeedthroughRadius");
  G4double L_down = L_up;
  G4double zPos_down = -zPos_up;
  G4Tubs *pWaterDisplacer_down = new G4Tubs("WaterDisplacer_down", 0, R_down,
                                            L_down, 0. * deg, 360. * deg);

  // Main volume
  G4double dLength = GetGeometryParameter("WaterDisplacerCylinderHeight");
  G4double R = GetGeometryParameter("WaterDisplacerCylinderRadius");
  // G4cout<<G4endl;
  // G4cout <<"WATER DISPLACER: R=" <<R<<" D="<<D<<G4endl;
  G4Tubs *pWaterDisplacer_main =
      new G4Tubs("WaterDisplacer_main", 0, R, dLength, 0. * deg, 360. * deg);

  // Merging
  G4UnionSolid *pWaterDisplacer_step =
      new G4UnionSolid("WaterDisplacer_step", pWaterDisplacer_main,
                       pWaterDisplacer_up, 0, G4ThreeVector(0., 0., zPos_up));
  G4UnionSolid *pWaterDisplacer = new G4UnionSolid(
      "WaterDisplacer", pWaterDisplacer_step, pWaterDisplacer_down, 0,
      G4ThreeVector(0., 0., zPos_down));

  // G4double zPos = GetGeometryParameter("OuterCryostatOffsetZ");
  // G4double zPos = GetGeometryParameter("z_nVetoOffset");
  m_pWaterDisplacerLogicalVolume = new G4LogicalVolume(
      pWaterDisplacer, WaterDisplacerMaterial, "WaterDisplacer", 0, 0, 0);
  m_pWaterDisplacerPhysicalVolume =
      new G4PVPlacement(0, G4ThreeVector(0. * mm, 0. * mm, 0. * mm),
                        m_pWaterDisplacerLogicalVolume, "WaterDisplacer",
                        m_pMotherLogicalVolume, false, 0);

  //---------------------------------------------------------------------------------------
  // DR 20161027
  G4OpticalSurface *OptLScintSurface = new G4OpticalSurface("LScintSurface");
  OptLScintSurface->SetType(dielectric_metal);  //(dielectric_LUT)
  OptLScintSurface->SetModel(unified);          //(LUT);
  OptLScintSurface->SetFinish(
      polished);  //(polishedvm2000air);  // Specular Reflector

  const G4int NUM = 7;

  G4double pp[NUM] = {2.034 * eV, 3.179 * eV, 3.351 * eV, 3.701 * eV,
                      4.065 * eV, 4.509 * eV, 4.959 * eV};
  G4double reflectivity_W[NUM] = {0.95, 0.95, 0.07, 0.05, 0.11, 0.06, 0.19};
  G4MaterialPropertiesTable *SMPT_W = new G4MaterialPropertiesTable();
  SMPT_W->AddProperty("REFLECTIVITY", pp, reflectivity_W, NUM);

  OptLScintSurface->SetMaterialPropertiesTable(SMPT_W);

  new G4LogicalBorderSurface("OuterLScintSurface", m_pVetoAcrylicPhysicalVolume,
                             m_pWaterDisplacerPhysicalVolume, OptLScintSurface);
  //---------------------------------------------------------------------------------------------------

  m_pMotherLogicalVolume = m_pWaterDisplacerLogicalVolume;
}
//=== End of Liquid Scintillator nVeto implementation ===

void Xenon1tDetectorConstruction::ConstructColumbiaCryostat1T() {
  if (m_iVerbosityLevel >= 1)
    G4cout << "Xenon1tDetectorConstruction::ConstructColumbiaCryostat1T "
              "Building cryostat geometry"
           << G4endl;

  G4Material *cryoMaterial = G4Material::GetMaterial(pCryostatMaterial);
  G4Material *Vacuum = G4Material::GetMaterial("Vacuum");

  //__________________________________________________________________________
  // G4double Rmax_cylinder_internal_lateral1 =
  // GetGeometryParameter("Rmax_cylinder_internal_lateral_pipe");
  // G4double Rmax_cylinder_internal_1 =
  // GetGeometryParameter("Rmax_cylinder_internal_central_pipe_1");
  // G4double Rmin_cylinder_internal_1 =
  // GetGeometryParameter("Rmin_cylinder_internal_central_pipe_1");
  // G4double Rmax_cylinder_internal_2 =
  // GetGeometryParameter("Rmax_cylinder_internal_central_pipe_2");
  // G4double Rmin_cylinder_internal_2 =
  // GetGeometryParameter("Rmin_cylinder_internal_central_pipe_2");
  // G4double Rmax_cylinder_internal_small1 =
  // GetGeometryParameter("Rmax_cylinder_internal_small_pipe1");
  // G4double Rmax_cylinder_small2 =
  // GetGeometryParameter("Rmax_cylinder_external_small_pipe2");
  // G4double Rmin_cylinder_small2 =
  // GetGeometryParameter("Rmin_cylinder_external_small_pipe2");
  //__________________________________________________________________________

  //==== The OUTER vessel ====

  // Outer hull
  G4double dLength = GetGeometryParameter("OuterCryostatCylinderHeight");
  G4double D = GetGeometryParameter("OuterCryostatOuterDiameter");
  G4double R0top = GetGeometryParameter("OuterCryostatR0top");
  G4double R1top = GetGeometryParameter("OuterCryostatR1top");
  G4double R0bot = GetGeometryParameter("OuterCryostatR0bot");
  G4double R1bot = GetGeometryParameter("OuterCryostatR1bot");
  G4double h_Flange = GetGeometryParameter("OuterCryostatFlangeHeight");
  G4double z_Flange = GetGeometryParameter("OuterCryostatFlangeZ");
  G4double dR_Flange = GetGeometryParameter("OuterCryostatFlangeThickness");

  G4UnionSolid *pOuterCryostatUnionSolid =
      ConstructVessel(D, dLength, R0top, R1top, R0bot, R1bot, 0, 0, dR_Flange,
                      h_Flange, z_Flange, 0, 0, 0, 0, 0, 0, true, false);

  if (m_iVerbosityLevel >= 1)
    G4cout << G4endl << "OUTER VESSEL: L=" << dLength << " D=" << D
           << " R0top=" << R0top << " R1top=" << R1top << " R0bot=" << R0bot
           << " R1bot=" << R1bot << G4endl;

  G4double zPos = GetGeometryParameter("OuterCryostatOffsetZ");

  // Holes
  G4double cylinder_height = 0.5 * 76.96 * mm;
  G4double cylinder_outer_radius =
      GetGeometryParameter("Rmax_cylinder_external_central_pipe");
  G4double cylinder_inner_radius =
      GetGeometryParameter("Rmax_cylinder_external_central_pipe") -
      GetGeometryParameter("Wall_thickness_central_external_pipe");
  G4Tubs *connection =
      new G4Tubs("Cylinder_external_SS", 0., cylinder_outer_radius,
                 cylinder_height, 0. * deg, 360. * deg);
  pOuterCryostatUnionSolid = new G4UnionSolid(
      "L", pOuterCryostatUnionSolid, connection, 0,
      G4ThreeVector(0, 0, (1259.14 + cylinder_height - 75.) * mm));

  G4double x_base_small = 450. * mm;
  G4double y_base_small = 260. * mm;
  G4double Rmax_cylinder_small_pipe = 0.5 * 120. * mm;
  G4double small_pipe_height = 0.5 * 190.1 * mm;
  G4Tubs *connection_small_pipe =
      new G4Tubs("Cylinder_lateral_SS", 0., Rmax_cylinder_small_pipe,
                 small_pipe_height, 0. * deg, 360. * deg);

  G4double HVFT_x_Offset = GetGeometryParameter("HVFT_x_Offset");
  G4double HVFT_y_Offset = GetGeometryParameter("HVFT_y_Offset");
  G4double HVFT_outer_radius = GetGeometryParameter("HVFT_OuterSS_Radius");
  //-450.333 -260.0
  G4ThreeVector Trans(HVFT_x_Offset, HVFT_y_Offset,
                      R0top * sin(atan(HVFT_y_Offset / HVFT_x_Offset)));

  pOuterCryostatUnionSolid = new G4UnionSolid(
      "L", pOuterCryostatUnionSolid, connection_small_pipe, 0,
      G4ThreeVector(x_base_small, y_base_small, 990. * mm + small_pipe_height));

  if (m_iVerbosityLevel >= 1) {
    G4cout << "***************************" << G4endl;
    G4cout << "***************************" << G4endl;
    G4cout << "**************   "
           << R0top * sin(atan(HVFT_y_Offset / HVFT_x_Offset)) << "    "
           << atan(1) << "    *************" << G4endl;
    G4cout << "***************************" << G4endl;
    G4cout << "***************************" << G4endl;
  }

  G4Tubs *hvft =
      new G4Tubs("hvft_tub", 0., HVFT_outer_radius, 50. * cm, 0, 2 * M_PI);

  G4SubtractionSolid *pOuterCryostatSubtractionSolid = new G4SubtractionSolid(
      "hvft_hole", pOuterCryostatUnionSolid, hvft, 0, Trans);

  //__________________________________________________________________________
  // G4double cylinder_radius_small1 = 0.5*105.*mm;
  // G4double cylinder_height_small1 = 0.5*169.3*mm;
  // G4Tubs *connection_small1= new G4Tubs("Cylinder_external_SS",0.,
  //   cylinder_radius_small1,cylinder_height_small1, 0.*deg, 360.*deg);
  // pOuterCryostatUnionSolid = new G4UnionSolid("L", pOuterCryostatUnionSolid,
  //   connection_small1,0,G4ThreeVector(450.331,-260.,1015.*mm));
  // G4double cylinder_radius_small2 = 0.5*39.8*mm;
  // G4double cylinder_height_small2 = 0.5*350.*mm;
  // G4Tubs *connection_small2= new G4Tubs("Cylinder_external_SS",0.,
  //   cylinder_radius_small2,cylinder_height_small2, 0.*deg, 360.*deg);
  // pOuterCryostatUnionSolid = new G4UnionSolid("L", pOuterCryostatUnionSolid,
  //   connection_small2,0,G4ThreeVector(-450.331,-260.,1015.*mm));
  // m_pOuterCryostatLogicalVolume = new
  //   G4LogicalVolume(pOuterCryostatUnionSolid, cryoMaterial,
  //   "OuterCryostatUnionSolid", 0, 0, 0);
  //__________________________________________________________________________

  m_pOuterCryostatLogicalVolume =
      new G4LogicalVolume(pOuterCryostatSubtractionSolid, cryoMaterial,
                          "OuterCryostatUnionSolid", 0, 0, 0);
  m_pOuterCryostatPhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(0. * mm, 0. * mm, zPos), m_pOuterCryostatLogicalVolume,
      "SS_OuterCryostat", m_pWaterLogicalVolume, false, 0);

  // Inner hull
  dLength = GetGeometryParameter("OuterCryostatCylinderHeight");
  D = GetGeometryParameter("OuterCryostatOuterDiameter") -
      2 * GetGeometryParameter("OuterCryostatThickness");
  R0top = GetGeometryParameter("OuterCryostatR0top") -
          GetGeometryParameter("OuterCryostatThicknessTop");
  R1top = GetGeometryParameter("OuterCryostatR1top") -
          GetGeometryParameter("OuterCryostatThicknessTop");
  R0bot = GetGeometryParameter("OuterCryostatR0bot") -
          GetGeometryParameter("OuterCryostatThicknessBot");
  R1bot = GetGeometryParameter("OuterCryostatR1bot") -
          GetGeometryParameter("OuterCryostatThicknessBot");
  G4double TopCor = GetGeometryParameter("OuterCryostatThicknessTop") -
                    GetGeometryParameter("OuterCryostatThickness");
  G4double BotCor = GetGeometryParameter("OuterCryostatThicknessBot") -
                    GetGeometryParameter("OuterCryostatThickness");

  if (m_iVerbosityLevel >= 1)
    G4cout << "ISO VACUUM  : L=" << dLength << " D=" << D << " R0top=" << R0top
           << " R1top=" << R1top << " R0bot=" << R0bot << " R1bot=" << R1bot
           << G4endl;

  // G4double cylinder_radius_v = 0.5*204.*mm;
  // G4double cylinder_height_v = 0.5*189.*mm;

  G4UnionSolid *pOuterCryostatVacuumUnionSolid = ConstructVessel(
      D, dLength, R0top, R1top, R0bot, R1bot, TopCor, BotCor, 0, 0,
      0, 0, 0, 0, 0, 0, 0, true, false);

  //__________________________________________________________________________
  // G4Tubs *connection_v= new G4Tubs("Cylinder_external_SS",0.,
  //   cylinder_radius_v,cylinder_height_v, 0.*deg, 360.*deg);
  // G4double cylinder_radius_center_small1_v = 0.5*102.*mm;
  // G4double cylinder_height_center_small1_v = 0.5*175.1*mm;
  // G4Tubs *connection_center_small1_v= new G4Tubs("Cylinder_external_SS",0.,
  //   cylinder_radius_center_small1_v,cylinder_height_center_small1_v, 0.*deg,
  //   360.*deg);
  // G4double cylinder_radius_center_small2_v = 0.5*38.*mm;
  // G4double cylinder_height_center_small2_v = 0.5*355.9*mm;
  // G4Tubs *connection_center_small2_v= new G4Tubs("Cylinder_external_SS",0.,
  //   cylinder_radius_center_small2_v, cylinder_height_center_small2_v,
  //   0.*deg, 360.*deg);
  // pOuterCryostatVacuumUnionSolid = new G4UnionSolid("L",
  //   pOuterCryostatVacuumUnionSolid, connection_v,0,
  //   G4ThreeVector(0,-375.,1025.*mm));
  // G4double cylinder_height_center_v = 0.5*5.*mm;
  // G4Tubs *connection_center_v= new G4Tubs("Cylinder_external_SS",0.,
  //   cylinder_inner_radius, cylinder_height_center_v, 0.*deg, 360.*deg);
  //__________________________________________________________________________

  G4Tubs *connection_center_v =
      new G4Tubs("Cylinder_external_SS", 0., cylinder_inner_radius,
                 cylinder_height, 0. * deg, 360. * deg);
  pOuterCryostatVacuumUnionSolid = new G4UnionSolid(
      "L", pOuterCryostatVacuumUnionSolid, connection_center_v, 0,
      G4ThreeVector(0, 0, 1259.14 * mm + cylinder_height - 75. * mm));

  G4double small_pipe_height_v = small_pipe_height + 13. * mm;
  G4double Rmin_cylinder_small_pipe = 0.5 * (120. - 3.) * mm;
  G4Tubs *samll_pipe_connection_v =
      new G4Tubs("Cylinder_small_SS", 0., Rmin_cylinder_small_pipe,
                 small_pipe_height_v, 0. * deg, 360. * deg);
  pOuterCryostatVacuumUnionSolid = new G4UnionSolid(
      "L", pOuterCryostatVacuumUnionSolid, samll_pipe_connection_v, 0,
      G4ThreeVector(x_base_small, y_base_small,
                    964. * mm + small_pipe_height_v));
  //__________________________________________________________________________
  // G4double cylinder_radius_lateral_v = 0.5*50.*mm;
  // G4double cylinder_height_lateral_v = 0.5*98.*mm;
  // G4Tubs *connection_lateral_v= new G4Tubs("Cylinder_extlateral_SS",0.,
  //   cylinder_radius_lateral_v,cylinder_height_lateral_v, 0.*deg, 360.*deg);
  // pOuterCryostatVacuumUnionSolid = new G4UnionSolid("L",
  //   pOuterCryostatVacuumUnionSolid, connection_lateral_v,0,
  //   G4ThreeVector(451.69*mm,451.69*mm,1090.*mm+cylinder_height_lateral_v));
  // pOuterCryostatVacuumUnionSolid = new G4UnionSolid("L",
  //   pOuterCryostatVacuumUnionSolid, connection_center_small2_v,0,
  //   G4ThreeVector(-450.331,-260.,1012.*mm));
  //__________________________________________________________________________

  m_pOuterCryostatVacuumLogicalVolume =
      new G4LogicalVolume(pOuterCryostatVacuumUnionSolid, Vacuum,
                          "OuterCryostatVacuumUnionSolid", 0, 0, 0);
  m_pOuterCryostatVacuumPhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(0., 0., 0.), m_pOuterCryostatVacuumLogicalVolume,
      "OuterCryostatVacuum", m_pOuterCryostatLogicalVolume, false, 0);

  //==== The INNER vessel ====

  // Outer hull
  dLength = GetGeometryParameter("InnerCryostatCylinderHeight");
  D = GetGeometryParameter("InnerCryostatOuterDiameter");
  R0top = GetGeometryParameter("InnerCryostatR0top");
  R1top = GetGeometryParameter("InnerCryostatR1top");
  R0bot = GetGeometryParameter("InnerCryostatR0bot");
  R1bot = GetGeometryParameter("InnerCryostatR1bot");
  h_Flange = GetGeometryParameter("InnerCryostatFlangeHeight");
  z_Flange = GetGeometryParameter("InnerCryostatFlangeZ");
  dR_Flange = GetGeometryParameter("InnerCryostatFlangeThickness");
  zPos = GetGeometryParameter("InnerCryostatOffsetZ");

  if (m_iVerbosityLevel >= 1)
    G4cout << G4endl << "INNER VESSEL: L=" << dLength << " D=" << D
           << " R0top=" << R0top << " R1top=" << R1top << " R0bot=" << R0bot
           << " R1bot=" << R1bot << G4endl;

  // Holes
  G4UnionSolid *pInnerCryostatUnionSolid =
      ConstructVessel(D, dLength, R0top, R1top, R0bot, R1bot, 0, 0, dR_Flange,
                      h_Flange, z_Flange, 0, 0, 0, 0, 0, 0, true, false);

  G4double cylinder_Outradius_innerConn =
      GetGeometryParameter("Rmax_cylinder_internal_central_big_pipe");
  // G4double cylinder_Innradius_innerConn =
  // GetGeometryParameter("Rmax_cylinder_internal_central_big_pipe") -
  // GetGeometryParameter("Wall_thickness_central_internal_big_pipe");
  G4double cylinder_height_inner = 0.5 * 217.78 * mm;
  G4Tubs *connection_inner =
      new G4Tubs("Cylinder_external_SS_4", 0., cylinder_Outradius_innerConn,
                 cylinder_height_inner, 0. * deg, 360. * deg);
  pInnerCryostatUnionSolid = new G4UnionSolid(
      "L", pInnerCryostatUnionSolid, connection_inner, 0,
      G4ThreeVector(0., 0., 994.32 * mm - 47. * mm + cylinder_height_inner));

  Rmax_cylinder_small_pipe = GetGeometryParameter("Rmax_cylinder_small_pipe");
  G4double small_pipe_height_inner = small_pipe_height + 0.5 * 178. * mm;
  G4Tubs *samll_pipe_connection_inner =
      new G4Tubs("Cylinder_small_SS", 0., Rmax_cylinder_small_pipe,
                 small_pipe_height_inner, 0. * deg, 360. * deg);
  pInnerCryostatUnionSolid = new G4UnionSolid(
      "L", pInnerCryostatUnionSolid, samll_pipe_connection_inner, 0,
      G4ThreeVector(x_base_small, y_base_small,
                    716. * mm + small_pipe_height_inner));

  //__________________________________________________________________________
  // G4RotationMatrix *RotPiece = new G4RotationMatrix;
  // RotPiece->rotateX(180.*deg);
  // G4double cylinder_height_inner_lateral=190.8*mm;
  // G4double cylinder_height_inner_small1=170.8*mm;
  // G4Tubs *connection_inner_lateral= new G4Tubs("Cylinder_external_SS_4ca",0.,
  // Rmax_cylinder_internal_lateral1, cylinder_height_inner_lateral, 0.*deg,
  // 360.*deg);
  // G4Tubs *connection_inner_small1= new G4Tubs("Cylinder_external_SS_4",0.,
  // Rmax_cylinder_internal_small1  ,cylinder_height_inner_small1, 0.*deg,
  // 360.*deg);

  // G4double cylinder_height_central_inner_air = 0.5*1936.8*mm;
  // G4double cylinder_radius_central_inner_air = 0.5*203.*mm;
  // G4Tubs *pcylinder_connection_air_central_inner = new
  // G4Tubs("Cylinder_vacuum", 0., cylinder_radius_central_inner_air,
  // cylinder_height_central_inner_air, 0.*deg, 360.*deg);
  // G4Tubs *pcylinder_connection_internal_1_central_inner = new
  // G4Tubs("Cylinder_internal_SS_1", Rmin_cylinder_internal_1,
  // Rmax_cylinder_internal_1, cylinder_height_central_inner_air, 0.*deg,
  // 360.*deg);
  // G4Tubs *pcylinder_connection_internal_2_central_inner = new
  // G4Tubs("Cylinder_internal_SS_2", Rmin_cylinder_internal_2,
  // Rmax_cylinder_internal_2, cylinder_height_central_inner_air, 0.*deg,
  // 360.*deg);

  // G4ThreeVector zTrans_central_inner_1(0*mm, 0.*mm,-200.*mm);

  // //right shape to fit with the inner cryostat. I need a subtraction solid

  // G4SubtractionSolid *connection_internal_1_central_inner = new
  // G4SubtractionSolid ("H", pcylinder_connection_internal_1_central_inner,
  // pInnerCryostatUnionSolid,0,zTrans_central_inner_1);
  // G4SubtractionSolid *connection_internal_2_central_inner = new
  // G4SubtractionSolid ("H", pcylinder_connection_internal_2_central_inner,
  // pInnerCryostatUnionSolid,0,zTrans_central_inner_1);
  // G4SubtractionSolid *connection_air_central_inner = new G4SubtractionSolid
  // ("H", pcylinder_connection_air_central_inner,
  // pInnerCryostatUnionSolid,0,zTrans_central_inner_1);

  // G4double cylinder_height_central_inner_small1 = 0.5*1757.1*mm;
  // G4ThreeVector zTrans_central_inner_small1(450.331*mm, -260.*mm,-200.*mm);
  // G4Tubs *pcylinder_connection_air_central_inner_small1 = new
  // G4Tubs("Cylinder_vacuum", 0.,Rmax_cylinder_internal_small1-1.5*mm,
  // cylinder_height_central_inner_small1, 0.*deg, 360.*deg);
  // G4SubtractionSolid *connection_air_central_inner_small1 = new
  // G4SubtractionSolid ("H", pcylinder_connection_air_central_inner_small1,
  // pInnerCryostatUnionSolid,0,zTrans_central_inner_small1);

  // G4double cylinder_height_central_inner_small2 = 0.5*1460.5*mm;
  // G4ThreeVector zTrans_central_inner_small2(-450.331*mm, -260.*mm,-200.*mm);
  // G4Tubs *pcylinder_connection_central_inner_small2_ss = new
  // G4Tubs("Cylinder_vacuum", Rmin_cylinder_small2, Rmax_cylinder_small2,
  // cylinder_height_central_inner_small2, 0.*deg, 360.*deg);
  // G4SubtractionSolid *connection_central_inner_small2_ss = new
  // G4SubtractionSolid ("H", pcylinder_connection_central_inner_small2_ss,
  // pInnerCryostatUnionSolid,0,zTrans_central_inner_small2);
  // G4LogicalVolume *connection_log_central_inner_small2_ss = new
  // G4LogicalVolume(connection_central_inner_small2_ss, SS316Ti, "A");
  // G4VPhysicalVolume *m_pconnection_phys_central_inner_small2_ss = new
  // G4PVPlacement(RotLat,
  // G4ThreeVector(-450.331,-260.,228.8),connection_log_central_inner_small2_ss,
  // "Carr_ss",m_pOuterCryostatVacuumLogicalVolume , false, 0);
  // pInnerCryostatUnionSolid = new G4UnionSolid("L", pInnerCryostatUnionSolid,
  // connection_inner_small1,0,G4ThreeVector(450.331,-260.,900.*mm));
  //__________________________________________________________________________

  m_pInnerCryostatLogicalVolume =
      new G4LogicalVolume(pInnerCryostatUnionSolid, cryoMaterial,
                          "InnerCryostatUnionSolid", 0, 0, 0);
  m_pInnerCryostatPhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(0, 0, zPos), m_pInnerCryostatLogicalVolume,
      "SS_InnerCryostat", m_pOuterCryostatVacuumLogicalVolume, false, 0);

  //__________________________________________________________________________
  // G4LogicalVolume *connection_log_air_central_inner = new
  // G4LogicalVolume(connection_air_central_inner, Vacuum, "A",0,0,0);
  // G4VPhysicalVolume *m_pconnection_phys_air_central_inner = new
  // G4PVPlacement(0,
  // G4ThreeVector(0,0,199.96*mm),connection_log_air_central_inner,
  // 							      "Vacuum_Central_pipe_inner",
  // m_pInnerCryostatLogicalVolume, false, 0);
  // G4LogicalVolume *connection_log_internal_1_central_inner = new
  // G4LogicalVolume(connection_internal_1_central_inner, SS316Ti,
  // "Adggd",0,0,0);
  // G4VPhysicalVolume *m_pconnection_phys_internal_1_central_inner = new
  // G4PVPlacement(0, G4ThreeVector(0,0,0.*mm),
  // 										     connection_log_internal_1_central_inner,
  // 										     "Vacuum_Central_Pipe_inner_1",connection_log_air_central_inner
  // ,
  // 										     false,
  // 0);
  // G4LogicalVolume *connection_log_internal_2_central_inner = new
  // G4LogicalVolume(connection_internal_2_central_inner, SS316Ti,
  // "Adhdhdh",0,0,0);
  // G4VPhysicalVolume *m_pconnection_phys_internal_2_central_inner = new
  // G4PVPlacement(0, G4ThreeVector(0,0,0.*mm),
  // 										     connection_log_internal_2_central_inner,
  // 										     "Vacuum_Central_Pipe_inner",connection_log_air_central_inner
  // ,
  // 										     false,
  // 0);
  // G4double cylinder_height_central_inner_lateral = 0.5*1791.39*mm;
  // G4double cylinder_radius_central_inner_lateral = 0.5*102.*mm;
  // G4Tubs *pcylinder_connection_central_inner_lateral= new
  // G4Tubs("Cylinder_external_SS",0., cylinder_radius_central_inner_lateral,
  // 								 cylinder_height_central_inner_lateral,
  // 0.*deg,
  // 360.*deg);
  // G4ThreeVector zTrans_central_inner_lateral(0*mm, -375.*mm,-200.*mm);
  // G4Tubs *pcylinder_connection_air_central_inner_lateral = new
  // G4Tubs("Cylinder_vacuum", 0., cylinder_radius_central_inner_lateral,
  // 								      cylinder_height_central_inner_lateral,
  // 0.*deg, 360.*deg);
  // G4SubtractionSolid *connection_central_inner_air_lateral = new
  // G4SubtractionSolid ("H", pcylinder_connection_air_central_inner_lateral,
  // 										     pInnerCryostatUnionSolid,0,zTrans_central_inner_lateral);

  // G4LogicalVolume *connection_log_air_central_inner_lateral = new
  // G4LogicalVolume(connection_central_inner_air_lateral, Vacuum, "A");
  // G4VPhysicalVolume *m_pconnection_phys_central_inner_air_lateral = new
  // G4PVPlacement(RotLat, G4ThreeVector(0,-375.,195.1*mm),
  // 										      connection_log_air_central_inner_lateral,
  // 										      "VAcuum_Pipe_lateral_inner",m_pInnerCryostatLogicalVolume,
  // false, 0);
  // G4LogicalVolume *connection_log_air_central_inner_small1 = new
  // G4LogicalVolume(connection_air_central_inner_small1, Vacuum, "A");
  // G4VPhysicalVolume *m_pconnection_phys_air_central_inner_small1 = new
  // G4PVPlacement(RotLat, G4ThreeVector(450.331*mm, -260.*mm,192.2*mm),
  // 										     connection_log_air_central_inner_small1,
  // 										     "Vacuum_Pipe_lateral_small_1",m_pInnerCryostatLogicalVolume,
  // false, 0);
  //__________________________________________________________________________

  //==== attributes ====
  G4Colour hSS316TiColor(0.600, 0.600, 0.600, 0.1);
  G4VisAttributes *pTitaniumVisAtt = new G4VisAttributes(hSS316TiColor);
  pTitaniumVisAtt->SetVisibility(true);
  m_pOuterCryostatLogicalVolume->SetVisAttributes(pTitaniumVisAtt);
  m_pOuterCryostatVacuumLogicalVolume->SetVisAttributes(
      G4VisAttributes::Invisible);
  m_pInnerCryostatLogicalVolume->SetVisAttributes(pTitaniumVisAtt);

  if (m_iVerbosityLevel >= 1)
    G4cout << "Xenon1tDetectorConstruction::ConstructColumbiaCryostat1T - "
              "Finished building cryostat geometry"
           << G4endl;
}

void Xenon1tDetectorConstruction::ConstructColumbiaCryostatNT() {
  if (m_iVerbosityLevel >= 1)
    G4cout << "Xenon1tDetectorConstruction::ConstructColumbiaCryostatNT - "
              "Building cryostat geometry"
           << G4endl;

  //==== Reflector covering outer cryostat ====

  const G4double dWaterLayerThickness = pOuterCryostatWaterLayerThickness;
  const G4double dFoilThickness = GetGeometryParameter("FoilThickness");
  G4Material* Tyvek = G4Material::GetMaterial("Tyvek");

  G4double dLength = GetGeometryParameter("OuterCryostatCylinderHeight");
  G4double D = GetGeometryParameter("OuterCryostatOuterDiameter") +
    2 * (dWaterLayerThickness + dFoilThickness);
  G4double R0top = GetGeometryParameter("OuterCryostatR0top") +
    dWaterLayerThickness + dFoilThickness;
  G4double R1top = GetGeometryParameter("OuterCryostatR1top") +
    dWaterLayerThickness + dFoilThickness;
  G4double R0bot = GetGeometryParameter("OuterCryostatR0bot") +
    dWaterLayerThickness + dFoilThickness;
  G4double R1bot = GetGeometryParameter("OuterCryostatR1bot") +
    dWaterLayerThickness + dFoilThickness;
  G4double h_Flange = GetGeometryParameter("OuterCryostatFlangeHeight") +
    2 * (dWaterLayerThickness + dFoilThickness);
  G4double dR_Flange = GetGeometryParameter("OuterCryostatFlangeThickness");
  G4double h_Ring1 = GetGeometryParameter("OuterCryostatRingsHeight") +
    2 * (dWaterLayerThickness + dFoilThickness);
  G4double dR_Ring1 = GetGeometryParameter("OuterCryostatRingsThickness");
  G4double h_Ring2 = h_Ring1;
  G4double dR_Ring2 = dR_Ring1;
  G4double z_Ring1 = -dLength * 0.5
     + GetGeometryParameter("OuterCryostatCylinderBaseToRing1BotZ")
     + (h_Ring1 - 2 * (dWaterLayerThickness + dFoilThickness)) * 0.5;
  G4double z_Ring2 = z_Ring1 + h_Ring1 * 0.5
     + GetGeometryParameter("OuterCryostatRing1TopToRing2BotZ")
     + (h_Ring2 - 4 * (dWaterLayerThickness + dFoilThickness)) * 0.5;
  G4double z_Flange = z_Ring2 + h_Ring2 * 0.5
     + GetGeometryParameter("OuterCryostatRing2TopToFlangeBotZ")
     + (h_Flange - 4 * (dWaterLayerThickness + dFoilThickness)) * 0.5;
  G4double zPos = GetGeometryParameter("OuterCryostatOffsetZ") -
                  GetGeometryParameter("z_nVetoOffset");

  G4UnionSolid *pOuterCryostatReflectorUnionSolid =
      ConstructVessel(D, dLength, R0top, R1top, R0bot, R1bot, 0, 0,
                      dR_Flange, h_Flange, z_Flange,
                      dR_Ring1, h_Ring1, z_Ring1,
                      dR_Ring2, h_Ring2, z_Ring2, true, false);

  m_pOuterCryostatReflectorLogicalVolume =
      new G4LogicalVolume(pOuterCryostatReflectorUnionSolid, Tyvek,
                          "OuterCryostatReflectorLogicalVolume", 0, 0, 0);
  m_pOuterCryostatReflectorPhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(0, 0, zPos), m_pOuterCryostatReflectorLogicalVolume,
      "OuterCryostatReflector", m_pWaterLogicalVolume, false, 0);

  //==== Water layer if its thickness > 0 ====

  if (dWaterLayerThickness > 0) {
    dLength = GetGeometryParameter("OuterCryostatCylinderHeight");
    D = GetGeometryParameter("OuterCryostatOuterDiameter") +
      2 * dWaterLayerThickness;
    R0top = GetGeometryParameter("OuterCryostatR0top") +
      dWaterLayerThickness;
    R1top = GetGeometryParameter("OuterCryostatR1top") +
      dWaterLayerThickness;
    R0bot = GetGeometryParameter("OuterCryostatR0bot") +
      dWaterLayerThickness;
    R1bot = GetGeometryParameter("OuterCryostatR1bot") +
      dWaterLayerThickness;
    h_Flange = GetGeometryParameter("OuterCryostatFlangeHeight") +
      2 * dWaterLayerThickness;
    dR_Flange = GetGeometryParameter("OuterCryostatFlangeThickness");
    h_Ring1 = GetGeometryParameter("OuterCryostatRingsHeight") +
      2 * dWaterLayerThickness;
    dR_Ring1 = GetGeometryParameter("OuterCryostatRingsThickness");
    h_Ring2 = h_Ring1;
    dR_Ring2 = dR_Ring1;
    z_Ring1 = -dLength * 0.5
       + GetGeometryParameter("OuterCryostatCylinderBaseToRing1BotZ")
       + (h_Ring1 - 2 * dWaterLayerThickness) * 0.5;
    z_Ring2 = z_Ring1 + h_Ring1 * 0.5
       + GetGeometryParameter("OuterCryostatRing1TopToRing2BotZ")
       + (h_Ring2 - 4 * dWaterLayerThickness) * 0.5;
    z_Flange = z_Ring2 + h_Ring2 * 0.5
       + GetGeometryParameter("OuterCryostatRing2TopToFlangeBotZ")
       + (h_Flange - 4 * dWaterLayerThickness) * 0.5;
    zPos = 0.;

    G4UnionSolid *pWaterLayerUnionSolid =
        ConstructVessel(D, dLength, R0top, R1top, R0bot, R1bot, 0, 0,
                        dR_Flange, h_Flange, z_Flange,
                        dR_Ring1, h_Ring1, z_Ring1,
                        dR_Ring2, h_Ring2, z_Ring2, true, false);

    m_pWaterLayerLogicalVolume =
      new G4LogicalVolume(pWaterLayerUnionSolid, Tyvek,
          "WaterLayerLogicalVolume", 0, 0, 0);
    m_pWaterLayerPhysicalVolume = new G4PVPlacement(
        0, G4ThreeVector(0, 0, zPos), m_pWaterLayerLogicalVolume,
        "WaterLayer", m_pOuterCryostatReflectorLogicalVolume, false, 0);
  } else if (dWaterLayerThickness < 0) {
    G4cerr << "Negative value is invalid for water layer thickness. "
      << "Set POSITIVE value!" << G4endl;
  }

  G4Material *cryoMaterial = G4Material::GetMaterial(pCryostatMaterial);
  G4Material *Vacuum = G4Material::GetMaterial("Vacuum");

  //==== OUTER vessel ====

  // Outer hull
  dLength = GetGeometryParameter("OuterCryostatCylinderHeight");
  D = GetGeometryParameter("OuterCryostatOuterDiameter");
  R0top = GetGeometryParameter("OuterCryostatR0top");
  R1top = GetGeometryParameter("OuterCryostatR1top");
  R0bot = GetGeometryParameter("OuterCryostatR0bot");
  R1bot = GetGeometryParameter("OuterCryostatR1bot");
  h_Flange = GetGeometryParameter("OuterCryostatFlangeHeight");
  dR_Flange = GetGeometryParameter("OuterCryostatFlangeThickness");
  h_Ring1 = GetGeometryParameter("OuterCryostatRingsHeight");
  dR_Ring1 = GetGeometryParameter("OuterCryostatRingsThickness");
  h_Ring2 = h_Ring1;
  dR_Ring2 = dR_Ring1;
  z_Ring1 = -dLength * 0.5
     + GetGeometryParameter("OuterCryostatCylinderBaseToRing1BotZ")
     + h_Ring1 * 0.5;
  z_Ring2 = z_Ring1 + h_Ring1 * 0.5
     + GetGeometryParameter("OuterCryostatRing1TopToRing2BotZ")
     + h_Ring2 * 0.5;
  z_Flange = z_Ring2 + h_Ring2 * 0.5
     + GetGeometryParameter("OuterCryostatRing2TopToFlangeBotZ")
     + h_Flange * 0.5;
  zPos = 0.;

  if (m_iVerbosityLevel >= 1)
    G4cout << "=== Outer vessel ===\n"
           << "  Cylinder height = " << dLength << ", diameter = " << D
           << G4endl << "  Spherical radius top = " << R0top << ", bottom = "
           << R0bot << G4endl << "  Toroidal radius top = " << R1top
           << ", bottom = " << R1bot << G4endl;

  G4UnionSolid *pOuterCryostatUnionSolid =
      ConstructVessel(D, dLength, R0top, R1top, R0bot, R1bot, 0, 0,
                      dR_Flange, h_Flange, z_Flange,
                      dR_Ring1, h_Ring1, z_Ring1,
                      dR_Ring2, h_Ring2, z_Ring2, true, false);
  m_pOuterCryostatLogicalVolume =
      new G4LogicalVolume(pOuterCryostatUnionSolid, cryoMaterial,
                          "OuterCryostatUnionSolid", 0, 0, 0);

  // mother volume of OuterCryostat depends on if WaterLayerThickness = 0 or not
  if (dWaterLayerThickness > 0) {
    m_pOuterCryostatPhysicalVolume = new G4PVPlacement(
        0, G4ThreeVector(0. * mm, 0. * mm, zPos), m_pOuterCryostatLogicalVolume,
        "SS_OuterCryostat", m_pWaterLayerLogicalVolume, false, 0);
  } else if (dWaterLayerThickness == 0) {
    m_pOuterCryostatPhysicalVolume = new G4PVPlacement(
        0, G4ThreeVector(0. * mm, 0. * mm, zPos), m_pOuterCryostatLogicalVolume,
        "SS_OuterCryostat", m_pOuterCryostatReflectorLogicalVolume, false, 0);
  }

  // Inner hull
  dLength = GetGeometryParameter("OuterCryostatCylinderHeight");
  D = GetGeometryParameter("OuterCryostatOuterDiameter") -
      2 * GetGeometryParameter("OuterCryostatThickness");
  R0top = GetGeometryParameter("OuterCryostatR0top") -
          GetGeometryParameter("OuterCryostatThicknessTop");
  R1top = GetGeometryParameter("OuterCryostatR1top") -
          GetGeometryParameter("OuterCryostatThicknessTop");
  R0bot = GetGeometryParameter("OuterCryostatR0bot") -
          GetGeometryParameter("OuterCryostatThicknessBot");
  R1bot = GetGeometryParameter("OuterCryostatR1bot") -
          GetGeometryParameter("OuterCryostatThicknessBot");
  G4double TopCor = GetGeometryParameter("OuterCryostatThicknessTop") -
                    GetGeometryParameter("OuterCryostatThickness");
  G4double BotCor = GetGeometryParameter("OuterCryostatThicknessBot") -
                    GetGeometryParameter("OuterCryostatThickness");

  if (m_iVerbosityLevel >= 1) {
    G4cout << "=== Outer vessel vacuum ===\n"
           << "  Cylinder height = " << dLength << ", diameter = " << D
           << G4endl << "  Spherical radius top = " << R0top << ", bottom = "
           << R0bot << G4endl << "  Toroidal radius top = " << R1top
           << ", bottom = " << R1bot << G4endl;
  }

  G4UnionSolid *pOuterCryostatVacuumUnionSolid = ConstructVessel(
      D, dLength, R0top, R1top, R0bot, R1bot, TopCor, BotCor,
      0, 0, 0, 0, 0, 0, 0, 0, 0, true, false);
  m_pOuterCryostatVacuumLogicalVolume =
      new G4LogicalVolume(pOuterCryostatVacuumUnionSolid, Vacuum,
                          "OuterCryostatVacuumUnionSolid", 0, 0, 0);
  m_pOuterCryostatVacuumPhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(0., 0., 0.), m_pOuterCryostatVacuumLogicalVolume,
      "OuterCryostatVacuum", m_pOuterCryostatLogicalVolume, false, 0);

  //==== INNER vessel ====

  // Outer hull
  dLength = GetGeometryParameter("InnerCryostatCylinderHeight");
  D = GetGeometryParameter("InnerCryostatOuterDiameter");
  R0top = GetGeometryParameter("InnerCryostatR0top");
  R1top = GetGeometryParameter("InnerCryostatR1top");
  R0bot = GetGeometryParameter("InnerCryostatR0bot");
  R1bot = GetGeometryParameter("InnerCryostatR1bot");
  h_Flange = GetGeometryParameter("InnerCryostatFlangeHeight");
  dR_Flange = GetGeometryParameter("InnerCryostatFlangeThickness");
  h_Ring1 = GetGeometryParameter("InnerCryostatRingsHeight");
  dR_Ring1 = GetGeometryParameter("InnerCryostatRingsThickness");
  h_Ring2 = h_Ring1;
  dR_Ring2 = dR_Ring1;
  z_Ring1 = -dLength * 0.5
     + GetGeometryParameter("InnerCryostatCylinderBaseToRing1BotZ")
     + h_Ring1 * 0.5;
  z_Ring2 = z_Ring1 + h_Ring1 * 0.5
     + GetGeometryParameter("InnerCryostatRing1TopToRing2BotZ")
     + h_Ring2 * 0.5;
  z_Flange = z_Ring2 + h_Ring2 * 0.5
     + GetGeometryParameter("InnerCryostatRing2TopToFlangeBotZ")
     + h_Flange * 0.5;
  zPos = GetGeometryParameter("InnerCryostatOffsetZ");

  if (m_iVerbosityLevel >= 1)
    G4cout << "=== Inner vessel ===\n"
           << "  Cylinder height = " << dLength << ", diameter = " << D
           << G4endl << "  Spherical radius top = " << R0top << ", bottom = "
           << R0bot << G4endl << "  Toroidal radius top = " << R1top
           << ", bottom = " << R1bot << G4endl;

  G4UnionSolid *pInnerCryostatUnionSolid =
      ConstructVessel(D, dLength, R0top, R1top, R0bot, R1bot, 0, 0,
                      dR_Flange, h_Flange, z_Flange,
                      dR_Ring1, h_Ring1, z_Ring1,
                      dR_Ring2, h_Ring2, z_Ring2, true, true);

  m_pInnerCryostatLogicalVolume =
      new G4LogicalVolume(pInnerCryostatUnionSolid, cryoMaterial,
                          "InnerCryostatUnionSolid", 0, 0, 0);
  m_pInnerCryostatPhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(0, 0, zPos), m_pInnerCryostatLogicalVolume,
      "SS_InnerCryostat", m_pOuterCryostatVacuumLogicalVolume, false, 0);

  // ePTFE reflectivity
  const G4int nePTFE = 2;
  G4double ppePTFE[nePTFE] = {1*eV, 5*eV};
  G4double dePTFEReflectivity[nePTFE] = {0.99, 0.99};
  G4MaterialPropertiesTable *pePTFEMPT = new G4MaterialPropertiesTable();
  pePTFEMPT->AddProperty("REFLECTIVITY", ppePTFE, dePTFEReflectivity, nePTFE);

  G4OpticalSurface *pOpePTFESurface = new G4OpticalSurface("OpePTFESurface");
  pOpePTFESurface->SetType(dielectric_dielectric);
  pOpePTFESurface->SetModel(unified);
  pOpePTFESurface->SetFinish(groundfrontpainted);
  pOpePTFESurface->SetMaterialPropertiesTable(pePTFEMPT);

  new G4LogicalBorderSurface("OuterCryostatReflectorSurface",
        m_pWaterPhysicalVolume,
        m_pOuterCryostatReflectorPhysicalVolume,
        pOpePTFESurface);

  //==== attributes ====
  G4Colour hSS316TiColor(0.600, 0.600, 0.600, 0.1);
  G4VisAttributes *pTitaniumVisAtt = new G4VisAttributes(hSS316TiColor);
  pTitaniumVisAtt->SetVisibility(true);  // Jakob
  m_pOuterCryostatLogicalVolume->SetVisAttributes(pTitaniumVisAtt);
  m_pOuterCryostatVacuumLogicalVolume->SetVisAttributes(pTitaniumVisAtt);
  m_pInnerCryostatLogicalVolume->SetVisAttributes(pTitaniumVisAtt);

  G4Colour hFoilColour(0.9, 0.9, 0.9, 0.1);
  G4VisAttributes *pFoilVisAtt = new G4VisAttributes(hFoilColour);
  pFoilVisAtt->SetVisibility(true);
  m_pOuterCryostatReflectorLogicalVolume->SetVisAttributes(pFoilVisAtt);

  if (m_iVerbosityLevel >= 1)
    G4cout << "Xenon1tDetectorConstruction::ConstructColumbiaCryostatNT - "
              "Finished building cryostat geometry"
           << G4endl;
}

void Xenon1tDetectorConstruction::ConstructVetoPMTArrays() {
  G4Material *Quartz = G4Material::GetMaterial("Quartz");
  G4Material *SS304LSteel = G4Material::GetMaterial("SS304LSteel");
  G4Material *Vacuum = G4Material::GetMaterial("Vacuum");
  G4Material *PhotoCathodeAluminium =
      G4Material::GetMaterial("PhotoCathodeAluminium");

  //======== 8" PMTs (Hamamatsu R5912-100-10-Y001) =========
  const G4double dPMTWindowOuterRadius =
      GetGeometryParameter("PMTWindowOuterRadius");
  const G4double dPMTWindowOuterHalfZ =
      GetGeometryParameter("PMTWindowOuterHalfZ");
  const G4double dPMTWindowTopZ = GetGeometryParameter("PMTWindowTopZ");
  const G4double dPMTPhotocathodeOuterRadius =
      GetGeometryParameter("PMTPhotocathodeOuterRadius");
  const G4double dPMTPhotocathodeOuterHalfZ =
      GetGeometryParameter("PMTPhotocathodeOuterHalfZ");
  const G4double dPMTPhotocathodeTopZ =
      GetGeometryParameter("PMTPhotocathodeTopZ");
  const G4double dPMTPhotocathodeInnerRadius =
      GetGeometryParameter("PMTPhotocathodeInnerRadius");
  const G4double dPMTPhotocathodeInnerHalfZ =
      GetGeometryParameter("PMTPhotocathodeInnerHalfZ");
  const G4double dPMTBodyOuterRadius =
      GetGeometryParameter("PMTBodyOuterRadius");
  const G4double dPMTBodyInnerRadius =
      GetGeometryParameter("PMTBodyInnerRadius");
  const G4double dPMTBodyHeight = GetGeometryParameter("PMTBodyHeight");
  const G4double dPMTBaseOuterRadius =
      GetGeometryParameter("PMTBaseOuterRadius");
  const G4double dPMTBaseInnerRadius =
      GetGeometryParameter("PMTBaseInnerRadius");
  const G4double dPMTBaseHeight = GetGeometryParameter("PMTBaseHeight");
  const G4double dPMTBaseInteriorHeight =
      GetGeometryParameter("PMTBaseInteriorHeight");

  //----- PMT window -----
  G4Ellipsoid *pPMTWindowEllipsoid = new G4Ellipsoid(
      "PMTWindowEllipsoid", dPMTWindowOuterRadius, dPMTWindowOuterRadius,
      dPMTWindowOuterHalfZ, -dPMTWindowOuterHalfZ, dPMTWindowTopZ);

  m_pPMTWindowLogicalVolume = new G4LogicalVolume(pPMTWindowEllipsoid, Quartz,
                                                  "PMTWindowVolume", 0, 0, 0);

  //----- PMT photocathode -----
  G4Ellipsoid *pPMTPhotocathodeEllipsoid =
      new G4Ellipsoid("PMTPhotocathodeEllipsoid", dPMTPhotocathodeOuterRadius,
                      dPMTPhotocathodeOuterRadius, dPMTPhotocathodeOuterHalfZ,
                      -dPMTPhotocathodeOuterHalfZ, dPMTPhotocathodeTopZ);

  m_pPMTPhotocathodeLogicalVolume =
      new G4LogicalVolume(pPMTPhotocathodeEllipsoid, PhotoCathodeAluminium,
                          "PMTPhotocathodeVolume", 0, 0, 0);

  m_pPMTPhotocathodePhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(0., 0., 0.), m_pPMTPhotocathodeLogicalVolume,
      "PMTPhotocathode", m_pPMTWindowLogicalVolume, false, 0);

  //----- PMT photocathode interior 1 -----
  G4Ellipsoid *pPMTPhotocathodeInterior1Ellipsoid = new G4Ellipsoid(
      "PMTPhotocathodeInterior1Ellipsoid", dPMTPhotocathodeInnerRadius,
      dPMTPhotocathodeInnerRadius, dPMTPhotocathodeInnerHalfZ,
      -dPMTPhotocathodeInnerHalfZ, dPMTPhotocathodeTopZ);

  m_pPMTPhotocathodeInterior1LogicalVolume =
      new G4LogicalVolume(pPMTPhotocathodeInterior1Ellipsoid, Vacuum,
                          "PMTPhotocathodeInterior1LogicalVolume", 0, 0, 0);

  m_pPMTPhotocathodeInterior1PhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(0., 0., 0.), m_pPMTPhotocathodeInterior1LogicalVolume,
      "PMTPhotocathodeInterior1", m_pPMTPhotocathodeLogicalVolume, false, 0);

  //----- PMT photocathode interior 2 -----
  G4Ellipsoid *pPMTPhotocathodeInterior2Ellipsoid = new G4Ellipsoid(
      "PMTPhotocathodeInterior2Ellipsoid", dPMTPhotocathodeInnerRadius,
      dPMTPhotocathodeInnerRadius, dPMTPhotocathodeInnerHalfZ,
      dPMTPhotocathodeTopZ, dPMTWindowTopZ);

  m_pPMTPhotocathodeInterior2LogicalVolume =
      new G4LogicalVolume(pPMTPhotocathodeInterior2Ellipsoid, Vacuum,
                          "PMTPhotocathodeInterior2LogicalVolume", 0, 0, 0);

  m_pPMTPhotocathodeInterior2PhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(0., 0., 0.), m_pPMTPhotocathodeInterior2LogicalVolume,
      "PMTPhotocathodeInterior2", m_pPMTWindowLogicalVolume, false, 0);

  //----- PMT body -----
  const G4double dPMTBodyHalfZ = 0.5 * dPMTBodyHeight;

  G4Tubs *pPMTBodyTubs = new G4Tubs("PMTBodyTubs", 0, dPMTBodyOuterRadius,
                                    dPMTBodyHalfZ, 0. * deg, 360. * deg);

  m_pPMTBodyLogicalVolume =
      new G4LogicalVolume(pPMTBodyTubs, SS304LSteel, "PMTBodyVolume", 0, 0, 0);

  //----- PMT body interior -----
  G4Tubs *pPMTBodyInteriorTubs =
      new G4Tubs("PMTBodyInteriorTubs", 0, dPMTBodyInnerRadius, dPMTBodyHalfZ,
                 0. * deg, 360. * deg);

  m_pPMTBodyInteriorLogicalVolume = new G4LogicalVolume(
      pPMTBodyInteriorTubs, Vacuum, "PMTBodyInteriorVolume", 0, 0, 0);

  m_pPMTBodyInteriorPhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(0., 0., 0.), m_pPMTBodyInteriorLogicalVolume,
      "PMTBodyInterior", m_pPMTBodyLogicalVolume, false, 0);

  //----- PMT base (not really the base but the bottom part of the PMT) -----
  const G4double dPMTBaseHalfZ = 0.5 * dPMTBaseHeight;

  G4Tubs *pPMTBaseTubs = new G4Tubs("PMTBaseTubs", 0, dPMTBaseOuterRadius,
                                    dPMTBaseHalfZ, 0. * deg, 360. * deg);

  m_pPMTBaseLogicalVolume =
      new G4LogicalVolume(pPMTBaseTubs, SS304LSteel, "PMTBaseVolume", 0, 0, 0);

  //----- PMT base interior -----
  const G4double dPMTBaseInteriorHalfZ = 0.5 * dPMTBaseInteriorHeight;

  G4Tubs *pPMTBaseInteriorTubs =
      new G4Tubs("PMTBaseInteriorTubs", 0, dPMTBaseInnerRadius,
                 dPMTBaseInteriorHalfZ, 0. * deg, 360. * deg);

  m_pPMTBaseInteriorLogicalVolume = new G4LogicalVolume(
      pPMTBaseInteriorTubs, Vacuum, "PMTBaseInteriorVolume", 0, 0, 0);

  m_pPMTBaseInteriorPhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(0., 0., 0.), m_pPMTBaseInteriorLogicalVolume,
      "PMTBaseInterior", m_pPMTBaseLogicalVolume, false, 0);

  //========== Placement Neutron Veto arrays ==========
  G4int iPmtHitNb;
  G4int iNbTopPMTs = (G4int)GetGeometryParameter("NbTopPMTs");
  G4int iNbBottomPMTs = (G4int)GetGeometryParameter("NbBottomPMTs");
  G4int iNbWaterPMTs = (G4int)GetGeometryParameter("NbWaterPMTs");
  G4int iNbLSPMTs = (G4int)GetGeometryParameter("NbLSPMTs");

  stringstream hVolumeName;

  for (G4int iPMTNb = iNbTopPMTs + iNbBottomPMTs;
       iPMTNb < iNbTopPMTs + iNbBottomPMTs + iNbLSPMTs; iPMTNb++) {
    iPmtHitNb = 20000 + iPMTNb - (iNbTopPMTs + iNbBottomPMTs);

    hVolumeName.str("");
    hVolumeName << "LSPMTWindowNo" << iPmtHitNb;

    m_hPMTWindowPhysicalVolumes.push_back(new G4PVPlacement(
        GetPMTRotation(iPMTNb), GetPMTPosition(iPMTNb, PMT_WINDOW),
        m_pPMTWindowLogicalVolume, hVolumeName.str(), m_pWaterLogicalVolume,
        false, iPmtHitNb));

    hVolumeName.str("");
    hVolumeName << "LSPMTBodyNo" << iPmtHitNb;

    m_hPMTBodyPhysicalVolumes.push_back(new G4PVPlacement(
        GetPMTRotation(iPMTNb), GetPMTPosition(iPMTNb, PMT_BODY),
        m_pPMTBodyLogicalVolume, hVolumeName.str(), m_pWaterLogicalVolume,
        false, iPmtHitNb));

    hVolumeName.str("");
    hVolumeName << "LSPMTBaseNo" << iPmtHitNb;

    m_hPMTBasePhysicalVolumes.push_back(new G4PVPlacement(
        GetPMTRotation(iPMTNb), GetPMTPosition(iPMTNb, PMT_BASE),
        m_pPMTBaseLogicalVolume, hVolumeName.str(), m_pWaterLogicalVolume,
        false, iPmtHitNb));
  }

  //========== Placement Water arrays ==========
  // top, bot, up, mid, under
  G4int iPmtOrderedIDs[84] = {
      78, 79, 80, 81, 82, 83, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70,
      71, 72, 73, 74, 75, 76, 77, 18, 19, 20, 21, 22, 23, 0,  1,  2,  3,
      4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 57, 58, 59,
      48, 49, 50, 51, 52, 53, 54, 55, 56, 45, 46, 47, 36, 37, 38, 39, 40,
      41, 42, 43, 44, 33, 34, 35, 24, 25, 26, 27, 28, 29, 30, 31, 32};

  for (G4int iPMTNb = iNbTopPMTs + iNbBottomPMTs + iNbLSPMTs;
       iPMTNb < iNbTopPMTs + iNbBottomPMTs + iNbLSPMTs + iNbWaterPMTs;
       iPMTNb++) {
    iPmtHitNb =
        10000 +
        iPmtOrderedIDs[iPMTNb - (iNbTopPMTs + iNbBottomPMTs + iNbLSPMTs)];

    hVolumeName.str("");
    hVolumeName << "WaterPMTWindowNo" << iPmtHitNb;

    m_hPMTWindowPhysicalVolumes.push_back(new G4PVPlacement(
        GetPMTRotation(iPMTNb), GetPMTPosition(iPMTNb, PMT_WINDOW),
        m_pPMTWindowLogicalVolume, hVolumeName.str(), m_pWaterLogicalVolume,
        false, iPmtHitNb));

    hVolumeName.str("");
    hVolumeName << "WaterPMTBodyNo" << iPmtHitNb;

    m_hPMTBodyPhysicalVolumes.push_back(new G4PVPlacement(
        GetPMTRotation(iPMTNb), GetPMTPosition(iPMTNb, PMT_BODY),
        m_pPMTBodyLogicalVolume, hVolumeName.str(), m_pWaterLogicalVolume,
        false, iPmtHitNb));

    hVolumeName.str("");
    hVolumeName << "WaterPMTBaseNo" << iPmtHitNb;

    m_hPMTBasePhysicalVolumes.push_back(new G4PVPlacement(
        GetPMTRotation(iPMTNb), GetPMTPosition(iPMTNb, PMT_BASE),
        m_pPMTBaseLogicalVolume, hVolumeName.str(), m_pWaterLogicalVolume,
        false, iPmtHitNb));
  }

  //========== Reflector for PMTs ==========
  G4OpticalSurface* OpPMTSurface = new G4OpticalSurface("PMTSurface");
  OpPMTSurface->SetType(dielectric_dielectric);
  OpPMTSurface->SetModel(unified);
  OpPMTSurface->SetFinish(groundfrontpainted);

  const G4int NUM = 2;
  G4double pp[NUM] = {1.*eV, 5.*eV};
  G4double reflectivity_W[NUM] = {0.99, 0.99};
  G4MaterialPropertiesTable* SMPT_W = new G4MaterialPropertiesTable();
  SMPT_W->AddProperty("REFLECTIVITY",pp,reflectivity_W,NUM);
  OpPMTSurface -> SetMaterialPropertiesTable(SMPT_W);

  for( size_t i=0; i<m_hPMTWindowPhysicalVolumes.size(); i++){
    new G4LogicalBorderSurface("PMTSurface",
                               m_hPMTWindowPhysicalVolumes.at(i),
                               m_pPMTPhotocathodeInterior2PhysicalVolume,
                               OpPMTSurface);
  }

  //PMT Body
  G4OpticalSurface* OpPMTBodySurface = new G4OpticalSurface("PMTBodySurface");
  OpPMTBodySurface->SetType(dielectric_dielectric);
  OpPMTBodySurface->SetModel(unified);
  OpPMTBodySurface->SetFinish(groundfrontpainted);

  G4double reflectivity_body[NUM] = {0.99, 0.99};
  G4MaterialPropertiesTable* SMPT_body = new G4MaterialPropertiesTable();
  SMPT_body->AddProperty("REFLECTIVITY",pp,reflectivity_body,NUM);
  OpPMTBodySurface -> SetMaterialPropertiesTable(SMPT_W);

  for( size_t i=0; i<m_hPMTBodyPhysicalVolumes.size(); i++){
    new G4LogicalBorderSurface("PMTSurface",
                               m_pWaterPhysicalVolume,
                               m_hPMTBodyPhysicalVolumes.at(i),
                               OpPMTBodySurface);
  }

  //PMT Base
  G4OpticalSurface* OpPMTBaseSurface = new G4OpticalSurface("PMTBaseSurface");
  OpPMTBaseSurface->SetType(dielectric_dielectric);
  OpPMTBaseSurface->SetModel(unified);
  OpPMTBaseSurface->SetFinish(groundfrontpainted);

  G4double reflectivity_base[NUM] = {0.99, 0.99};
  G4MaterialPropertiesTable* SMPT_base = new G4MaterialPropertiesTable();
  SMPT_base->AddProperty("REFLECTIVITY",pp,reflectivity_base,NUM);
  OpPMTBaseSurface -> SetMaterialPropertiesTable(SMPT_W);

  for( size_t i=0; i<m_hPMTBasePhysicalVolumes.size(); i++){
    new G4LogicalBorderSurface("PMTBaseSurface",
                               m_pWaterPhysicalVolume,
                               m_hPMTBasePhysicalVolumes.at(i),
                               OpPMTBaseSurface);
  }

  //========== PMT sensitivity ==========
  G4SDManager *pSDManager = G4SDManager::GetSDMpointer();

  if (pSDManager->GetCollectionID("PmtHitsCollection") == -1) {
    // Add new sensitive det
    pPmtSD = new Xenon1tPmtSensitiveDetector("Xenon1t/PmtSD");
    pSDManager->AddNewDetector(pPmtSD);
    m_pPMTPhotocathodeLogicalVolume->SetSensitiveDetector(pPmtSD);
  } else {
    // use old sensitive det
    G4VSensitiveDetector *pPmtSD =
        pSDManager->FindSensitiveDetector("Xenon1t/PmtSD", true);
    m_pPMTPhotocathodeLogicalVolume->SetSensitiveDetector(pPmtSD);
  }

  G4SDManager *pSDWindowManager = G4SDManager::GetSDMpointer();

  if (pSDWindowManager->GetCollectionID("PmtWindowHitsCollection") == -1) {
    // Add new sensitive det
    pPmtWindowSD = new Xenon1tPmtWindowSensitiveDetector("Xenon1t/PmtWindowSD");
    pSDWindowManager->AddNewDetector(pPmtWindowSD);
    m_pPMTWindowLogicalVolume->SetSensitiveDetector(pPmtWindowSD);
  } else {
    // use old sensitive det
    G4VSensitiveDetector *pPmtWindowSD =
        pSDWindowManager->FindSensitiveDetector("Xenon1t/PmtWindowSD", true);
    m_pPMTWindowLogicalVolume->SetSensitiveDetector(pPmtWindowSD);
  }

  //==== attributes ====
  m_pPMTPhotocathodeInterior1LogicalVolume->SetVisAttributes(
      G4VisAttributes::Invisible);
  m_pPMTPhotocathodeInterior2LogicalVolume->SetVisAttributes(
      G4VisAttributes::Invisible);
  m_pPMTBodyInteriorLogicalVolume->SetVisAttributes(G4VisAttributes::Invisible);
  m_pPMTBaseInteriorLogicalVolume->SetVisAttributes(G4VisAttributes::Invisible);

  G4Colour hPMTWindowColor(1., 0.757, 0.024);
  G4VisAttributes *pPMTWindowVisAtt = new G4VisAttributes(hPMTWindowColor);
  pPMTWindowVisAtt->SetVisibility(true);
  m_pPMTWindowLogicalVolume->SetVisAttributes(pPMTWindowVisAtt);

  G4Colour hPMTPhotocathodeColor(1., 0.082, 0.011);
  G4VisAttributes *pPMTPhotocathodeVisAtt =
      new G4VisAttributes(hPMTPhotocathodeColor);
  pPMTPhotocathodeVisAtt->SetVisibility(true);
  m_pPMTPhotocathodeLogicalVolume->SetVisAttributes(pPMTPhotocathodeVisAtt);

  G4Colour hPMTCasingColor(1., 0.486, 0.027);
  G4VisAttributes *pPMTCasingVisAtt = new G4VisAttributes(hPMTCasingColor);
  pPMTCasingVisAtt->SetVisibility(true);
  m_pPMTBodyLogicalVolume->SetVisAttributes(pPMTCasingVisAtt);
  m_pPMTBaseLogicalVolume->SetVisAttributes(pPMTCasingVisAtt);
}

void Xenon1tDetectorConstruction::ConstructFoilCylinder() {
  // cylindrical foil wall
  const G4double dFoilThickness = GetGeometryParameter("FoilThickness");
  const G4double dFoilCylinderInnerRadius = GetGeometryParameter("FoilCylinderInnerRadius");
  const G4double dFoilCylinderOuterRadius = GetGeometryParameter("FoilCylinderOuterRadius");
  const G4double dFoilCylinderHeight = GetGeometryParameter("FoilCylinderHeight");
  G4Tubs *pFoilTubs = new G4Tubs("FoilTubs", dFoilCylinderInnerRadius, dFoilCylinderOuterRadius,
      dFoilCylinderHeight/2., 0, 360.*deg);

  G4Material *Tyvek = G4Material::GetMaterial("Tyvek");
  m_pFoilCylinderLogicalVolume = new G4LogicalVolume(pFoilTubs, Tyvek, "FoilCylinderLogicalVolume");

  const G4double dFoilX = 0;
  const G4double dFoilY = 0;
  const G4double dFoilZ = 736.*mm;
  m_pFoilCylinderPhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(dFoilX, dFoilY, dFoilZ), "FoilCylinder",
      m_pFoilCylinderLogicalVolume, m_pWaterPhysicalVolume, false, 0);

  // cylindrical foil top/bottom lid
  G4Tubs *pFoilCylinderLidTubs = new G4Tubs("FoilCylinderLidTubs", 0, dFoilCylinderInnerRadius,
      dFoilThickness/2., 0, 360.*deg);

  m_pFoilCylinderLidLogicalVolume = new G4LogicalVolume(pFoilCylinderLidTubs, Tyvek, "FoilCylinderLidLogicalVolume");

  const G4double dFoilTopLidX = 0;
  const G4double dFoilTopLidY = 0;
  const G4double dFoilTopLidZ = dFoilZ + dFoilCylinderHeight/2. - dFoilThickness/2.;
  m_pFoilCylinderTopLidPhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(dFoilTopLidX, dFoilTopLidY, dFoilTopLidZ), "FoilCylinderTopLid",
      m_pFoilCylinderLidLogicalVolume, m_pWaterPhysicalVolume, false, 0);

  const G4double dFoilBottomLidX = 0;
  const G4double dFoilBottomLidY = 0;
  const G4double dFoilBottomLidZ = dFoilZ - dFoilCylinderHeight/2. + dFoilThickness/2.- 800.*mm;
  m_pFoilCylinderBottomLidPhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(dFoilBottomLidX, dFoilBottomLidY, dFoilBottomLidZ), "FoilCylinderBottomLid",
      m_pFoilCylinderLidLogicalVolume, m_pWaterPhysicalVolume, false, 0);

  // foil cylinder lower side
  const G4double dFoilCylinderLowerSideHeight = GetGeometryParameter("FoilCylinderLowerSideHeight");
  G4Tubs *pFoilCylinderLowerSide = new G4Tubs("FoilCylinderLowerSideTubs", dFoilCylinderInnerRadius, dFoilCylinderOuterRadius,
      dFoilCylinderLowerSideHeight/2., 0, 360.*deg);

  m_pFoilCylinderLowerSideLogicalVolume = new G4LogicalVolume(pFoilCylinderLowerSide, Tyvek, "FoilCylinderLowerSideLogicalVolume");

  const G4double dFoilLowerSideX = 0;
  const G4double dFoilLowerSideY = 0;
  const G4double dFoilLowerSideZ = dFoilBottomLidZ + dFoilCylinderLowerSideHeight/2.;
  m_pFoilCylinderLowerSidePhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(dFoilLowerSideX, dFoilLowerSideY, dFoilLowerSideZ), "FoilCylinderLowerSide",
      m_pFoilCylinderLowerSideLogicalVolume, m_pWaterPhysicalVolume, false, 0);

  // reflectivity
  const G4int NUM = 2;
  G4double pp[NUM] = {1*eV, 5*eV};
  G4double dReflectivity[NUM] = {0.99, 0.99};
  G4MaterialPropertiesTable *pFoilMPT = new G4MaterialPropertiesTable();
  pFoilMPT->AddProperty("REFLECTIVITY", pp, dReflectivity, NUM);

  G4OpticalSurface *pOpFoilSurface = new G4OpticalSurface("OpFoilSurface");
  pOpFoilSurface->SetType(dielectric_dielectric);
  pOpFoilSurface->SetModel(unified);
  pOpFoilSurface->SetFinish(groundfrontpainted);
  pOpFoilSurface->SetMaterialPropertiesTable(pFoilMPT);

  new G4LogicalBorderSurface("FoilCylinderSurface",
      m_pWaterPhysicalVolume, m_pFoilCylinderPhysicalVolume, pOpFoilSurface);
  new G4LogicalBorderSurface("FoilCylinderTopLidSurface",
      m_pWaterPhysicalVolume, m_pFoilCylinderTopLidPhysicalVolume, pOpFoilSurface);
  new G4LogicalBorderSurface("FoilCylinderBottomLidSurface",
      m_pWaterPhysicalVolume, m_pFoilCylinderBottomLidPhysicalVolume, pOpFoilSurface);
  new G4LogicalBorderSurface("FoilCylinderLowerSideSurface",
      m_pWaterPhysicalVolume, m_pFoilCylinderLowerSidePhysicalVolume, pOpFoilSurface);

  // attributes
  G4Colour hFoilColour(0.6, 0.6, 0.6, 0.1);
  G4VisAttributes *pFoilVisAtt = new G4VisAttributes(hFoilColour);
  pFoilVisAtt->SetVisibility(true);
  m_pFoilCylinderLogicalVolume->SetVisAttributes(pFoilVisAtt);
  m_pFoilCylinderLidLogicalVolume->SetVisAttributes(pFoilVisAtt);
  m_pFoilCylinderLowerSideLogicalVolume->SetVisAttributes(pFoilVisAtt);
}

void Xenon1tDetectorConstruction::ConstructFoilBox() {
  // box foil wall
  const G4double dFoilThickness = GetGeometryParameter("FoilThickness");
  const G4double dFoilSidePanelWidth = GetGeometryParameter("FoilSidePanelWidth");
  const G4double dFoilSidePanelHeight = GetGeometryParameter("FoilSidePanelHeight");
  G4Box *pFoilBoxSidePanelBox = new G4Box("FoilBoxSidePanelBox", 0.5 *dFoilSidePanelWidth, 0.5 * dFoilThickness, 0.5 * dFoilSidePanelHeight);

  // cut holes for PMT
  G4Tubs* pHoleForPMTBox = new G4Tubs("HoleForPMTBox", 0, 45*mm, 10*mm, 0, 360*deg);
  const G4int iNbLSPMTs = GetGeometryParameter("NbLSPMTs");
  const G4int iNbLSSidePMTColumnsPerPanel = GetGeometryParameter("NbLSSidePMTColumns") / 4;
  const G4int iNbLSSidePMTRows = GetGeometryParameter("NbLSSidePMTRows");

  G4RotationMatrix *pHoleRotation = new G4RotationMatrix();
  pHoleRotation->rotateX(90.*deg);
  G4SubtractionSolid* pFoilBoxSidePanelWithHolesBox;
  for (G4int iHole = 0; iHole < iNbLSPMTs / 4; ++iHole) {
    const G4double dHoleX = (iHole % iNbLSSidePMTColumnsPerPanel) / (iNbLSSidePMTColumnsPerPanel - 1.) * 3200*mm - 1600*mm;
    const G4double dHoleY = 0;
    const G4double dHoleZ = (iHole % iNbLSSidePMTRows) / (iNbLSSidePMTRows - 1.) * 2598*mm - 1474*mm;
    if (iHole == 0) {
      pFoilBoxSidePanelWithHolesBox = new G4SubtractionSolid("FoilBoxSidePanelWithHolesBox",
          pFoilBoxSidePanelBox, pHoleForPMTBox, pHoleRotation, G4ThreeVector(dHoleX, dHoleY, dHoleZ));
    } else {
      pFoilBoxSidePanelWithHolesBox = new G4SubtractionSolid("FoilBoxSidePanelWithHolesBox",
          pFoilBoxSidePanelWithHolesBox, pHoleForPMTBox, pHoleRotation, G4ThreeVector(dHoleX, dHoleY, dHoleZ));
    }
  }

  G4Material *Tyvek = G4Material::GetMaterial("Tyvek");
  m_pFoilBoxSidePanelLogicalVolume = new G4LogicalVolume(pFoilBoxSidePanelWithHolesBox, Tyvek, "FoilBoxSidePanelLogicalVolume");

  const G4double dFoilSidePanelZ = 376.*mm;
  m_pFoilBoxSidePanelPhysicalVolume[0] = new G4PVPlacement(
      0, G4ThreeVector(-0.5 * dFoilThickness, -0.5 * dFoilSidePanelWidth, dFoilSidePanelZ), "FoilBoxSidePanel1",
        m_pFoilBoxSidePanelLogicalVolume, m_pWaterPhysicalVolume, false, 0);
  m_pFoilBoxSidePanelPhysicalVolume[1] = new G4PVPlacement(
      0, G4ThreeVector(0.5 * dFoilThickness, 0.5 * dFoilSidePanelWidth, dFoilSidePanelZ), "FoilBoxSidePanel2",
        m_pFoilBoxSidePanelLogicalVolume, m_pWaterPhysicalVolume, false, 0);
  G4RotationMatrix *pFoilSidePanelRotation = new G4RotationMatrix();
  pFoilSidePanelRotation->rotateZ(90.*deg);
  m_pFoilBoxSidePanelPhysicalVolume[2] = new G4PVPlacement(
      pFoilSidePanelRotation, G4ThreeVector(0.5 * dFoilSidePanelWidth, -0.5 * dFoilThickness, dFoilSidePanelZ), "FoilBoxSidePanel3",
        m_pFoilBoxSidePanelLogicalVolume, m_pWaterPhysicalVolume, false, 0);
  m_pFoilBoxSidePanelPhysicalVolume[3] = new G4PVPlacement(
      pFoilSidePanelRotation, G4ThreeVector(-0.5 * dFoilSidePanelWidth, 0.5 * dFoilThickness, dFoilSidePanelZ), "FoilBoxSidePanel4",
        m_pFoilBoxSidePanelLogicalVolume, m_pWaterPhysicalVolume, false, 0);

  // box foil top
  G4Box *pFoilBoxTopPanelBox = new G4Box("FoilBoxTopPanelBox",
      0.5 * (dFoilSidePanelWidth - dFoilThickness), 0.5 * (dFoilSidePanelWidth - dFoilThickness), 0.5 * dFoilThickness);

  // cut holes for calibration interfaces
  G4Box *pHoleForIBeltBox = new G4Box("HoleForIBeltBox", 11.*cm, 11.*cm, 10.*mm);
  G4Tubs *pHoleForNGTubs = new G4Tubs("HoleForNGTubs", 0, 10.*cm, 10.*cm, 0, 360.*deg);
  G4SubtractionSolid* pFoilBoxTopPanelWithHoles;
  pFoilBoxTopPanelWithHoles = new G4SubtractionSolid("FoilBoxTopPanelWithHoles", pFoilBoxTopPanelBox, pHoleForIBeltBox,
      0, G4ThreeVector(906.*mm, 423.*mm, 0));
  pFoilBoxTopPanelWithHoles = new G4SubtractionSolid("FoilBoxTopPanelWithHoles", pFoilBoxTopPanelWithHoles, pHoleForNGTubs,
      0, G4ThreeVector(835.*mm, -390.*mm, 0));
  pFoilBoxTopPanelWithHoles = new G4SubtractionSolid("FoilBoxTopPanelWithHoles", pFoilBoxTopPanelWithHoles, pHoleForNGTubs,
      0, G4ThreeVector(316.*mm, 868.*mm, 0));
  pFoilBoxTopPanelWithHoles = new G4SubtractionSolid("FoilBoxTopPanelWithHoles", pFoilBoxTopPanelWithHoles, pHoleForNGTubs,
      0, G4ThreeVector(-924.*mm, 0, 0));

  m_pFoilBoxTopPanelLogicalVolume = new G4LogicalVolume(pFoilBoxTopPanelWithHoles, Tyvek, "FoilBoxTopPanelLogicalVolume");

  const G4double dFoilTopPanelX = 0;
  const G4double dFoilTopPanelY = 0;
  const G4double dFoilTopPanelZ = dFoilSidePanelZ + 0.5 * dFoilSidePanelHeight - 0.5 * dFoilThickness;
  m_pFoilBoxTopPanelPhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(dFoilTopPanelX, dFoilTopPanelY, dFoilTopPanelZ), "FoilBoxTopPanel",
      m_pFoilBoxTopPanelLogicalVolume, m_pWaterPhysicalVolume, false, 0);

  // box foil bottom
  G4Box *pFoilBoxBottomPanelBox = new G4Box("FoilBoxBottomPanelBox",
      0.5 * (dFoilSidePanelWidth - dFoilThickness), 0.5 * (dFoilSidePanelWidth - dFoilThickness), 0.5 * dFoilThickness);

  m_pFoilBoxBottomPanelLogicalVolume = new G4LogicalVolume(pFoilBoxBottomPanelBox, Tyvek, "FoilBoxBottomPanelLogicalVolume");

  const G4double dFoilBottomPanelX = 0;
  const G4double dFoilBottomPanelY = 0;
  const G4double dFoilBottomPanelZ = dFoilSidePanelZ - 0.5 * dFoilSidePanelHeight + 0.5 * dFoilThickness;
  m_pFoilBoxBottomPanelPhysicalVolume = new G4PVPlacement(
      0, G4ThreeVector(dFoilBottomPanelX, dFoilBottomPanelY, dFoilBottomPanelZ), "FoilBoxBottomPanel",
      m_pFoilBoxBottomPanelLogicalVolume, m_pWaterPhysicalVolume, false, 0);

  // reflectivity
  const G4int NUM = 2;
  G4double pp[NUM] = {1*eV, 5*eV};
  G4double dReflectivity[NUM] = {0.99, 0.99};
  G4MaterialPropertiesTable *pFoilMPT = new G4MaterialPropertiesTable();
  pFoilMPT->AddProperty("REFLECTIVITY", pp, dReflectivity, NUM);

  G4OpticalSurface *pOpFoilSurface = new G4OpticalSurface("OpFoilSurface");
  pOpFoilSurface->SetType(dielectric_dielectric);
  pOpFoilSurface->SetModel(unified);
  pOpFoilSurface->SetFinish(groundfrontpainted);
  pOpFoilSurface->SetMaterialPropertiesTable(pFoilMPT);

  new G4LogicalBorderSurface("FoilBoxSidePanel1Surface",
      m_pWaterPhysicalVolume, m_pFoilBoxSidePanelPhysicalVolume[0], pOpFoilSurface);
  new G4LogicalBorderSurface("FoilBoxSidePanel2Surface",
      m_pWaterPhysicalVolume, m_pFoilBoxSidePanelPhysicalVolume[1], pOpFoilSurface);
  new G4LogicalBorderSurface("FoilBoxSidePanel3Surface",
      m_pWaterPhysicalVolume, m_pFoilBoxSidePanelPhysicalVolume[2], pOpFoilSurface);
  new G4LogicalBorderSurface("FoilBoxSidePanel4Surface",
      m_pWaterPhysicalVolume, m_pFoilBoxSidePanelPhysicalVolume[3], pOpFoilSurface);
  new G4LogicalBorderSurface("FoilBoxTopPanelSurface",
      m_pWaterPhysicalVolume, m_pFoilBoxTopPanelPhysicalVolume, pOpFoilSurface);
  new G4LogicalBorderSurface("FoilBoxBottomPanelSurface",
      m_pWaterPhysicalVolume, m_pFoilBoxBottomPanelPhysicalVolume, pOpFoilSurface);

  // attributes
  G4Colour hFoilColour(0.6, 0.6, 0.6, 0.1);
  G4VisAttributes *pFoilVisAtt = new G4VisAttributes(hFoilColour);
  pFoilVisAtt->SetVisibility(true);
  m_pFoilBoxSidePanelLogicalVolume->SetVisAttributes(pFoilVisAtt);
  m_pFoilBoxTopPanelLogicalVolume->SetVisAttributes(pFoilVisAtt);
  m_pFoilBoxBottomPanelLogicalVolume->SetVisAttributes(pFoilVisAtt);
}



void Xenon1tDetectorConstruction::ConstructFoilOctagon()
{ // Pietro 190709
  G4Material *Tyvek = G4Material::GetMaterial("Tyvek");
  const G4double dFoilThickness = GetGeometryParameter("FoilThickness");

  //====== Lateral reflector ======
  const G4double dOctagonApothema = 0.5 * GetGeometryParameter("FoilOctagonSideLength") + 0.5 * dFoilThickness; // orthogonal distance from inner surface of lateral panels
  const G4double dFoilSidePanelWidth = GetGeometryParameter("FoilOctagonSidePanelWidth");
  const G4double dFoilSidePanelHeight = GetGeometryParameter("FoilOctagonSidePanelHeight");
  const G4double zPlane[]={-0.5*dFoilSidePanelHeight,0.5*dFoilSidePanelHeight};
  const G4double rInner[]={dOctagonApothema-0.5*dFoilThickness,dOctagonApothema-0.5*dFoilThickness};
  const G4double rOuter[]={dOctagonApothema+0.5*dFoilThickness,dOctagonApothema+0.5*dFoilThickness};
  G4Polyhedra *pNVetoLateralReflector = new G4Polyhedra("nVetoLateralReflector",0,2*M_PI,8,2,zPlane,rInner,rOuter);
  const G4double dFoilSidePanelZ = 376.*mm;

  //------ Holes for PMTs ------
  G4Tubs* pHoleForPMT = new G4Tubs("HoleForPMT", 0, 101.*mm, 10.*mm, 0, 360*deg);
  const G4int iNbLSPMTs = GetGeometryParameter("NbLSPMTs");
  const G4int rows = GetGeometryParameter("NbLSSidePMTRows");
  const G4int cols_alongSS = GetGeometryParameter("NbLSSidePMTColumns_SideAlongSS");
  const G4int cols_diagonal = GetGeometryParameter("NbLSSidePMTColumns_DiagonalSide");
  const G4double y_max = GetGeometryParameter("DistanceBetweenColumns_SideAlongSS");
  const G4double y_min = -1. * GetGeometryParameter("DistanceBetweenColumns_SideAlongSS");
  const G4double dist = 0.5 * GetGeometryParameter("DistanceBetweenColumns_DiagonalSide");
  const G4double z_max = GetGeometryParameter("NVetoPMTTopRowZ");
  const G4double z_min = GetGeometryParameter("NVetoPMTBottomRowZ");
  G4int j;
  G4double angle;
  G4double dAngleOffset=22.5*deg;
  G4int colId;
  G4int rowId;
  G4int sideId;
  G4SubtractionSolid* pNVetoLateralReflectorWithHoles;

  for (G4int i=0; i<iNbLSPMTs; i++){
    G4ThreeVector hPosHole;
    G4RotationMatrix* pHoleRotation = new G4RotationMatrix();
    pHoleRotation->rotateX(90.*deg);
    if(i<(rows*cols_alongSS*4)){ //Sides along Nikhef support structure (3 PMT columns, 6 rows)
      rowId = (i/4) / cols_alongSS;
      colId = (i/4) % cols_alongSS;
      sideId = i % 4;
      hPosHole.setX(dOctagonApothema);
      hPosHole.setY(y_min+colId/(cols_alongSS-1.)*(y_max-y_min));
      hPosHole.setZ(z_min+rowId/(rows-1.)*(z_max-z_min) + 900.*mm -dFoilSidePanelZ);
      hPosHole.rotateZ(sideId*90.*deg + dAngleOffset);
      pHoleRotation->rotateY(dAngleOffset + 90.*deg + sideId*90.*deg);
    } else { //Diagonal sides (2 PMT columns, 6 rows)
      j=i-rows*cols_alongSS*4;
      rowId = (j/4) / cols_diagonal;
      colId = (j/4) % cols_diagonal;
      sideId = j % 4;
      angle = 45.*deg + sideId*90.*deg + dAngleOffset;
      hPosHole.setX(dOctagonApothema*cos(angle) + dist*cos(angle+90.*deg+colId*180.*deg));
      hPosHole.setY(dOctagonApothema*sin(angle) + dist*sin(angle+90.*deg+colId*180.*deg));
      hPosHole.setZ(z_min+rowId/(rows-1.)*(z_max-z_min) + 900.*mm -dFoilSidePanelZ);
      pHoleRotation->rotateY(dAngleOffset + 90.*deg + sideId*90.*deg + 45.*deg);
    }
    if (i == 0) {
      pNVetoLateralReflectorWithHoles = new G4SubtractionSolid("nVetoLateralReflectorWithHoles", pNVetoLateralReflector, pHoleForPMT, pHoleRotation, hPosHole);
    } else {
      pNVetoLateralReflectorWithHoles = new G4SubtractionSolid("nVetoLateralReflectorWithHoles", pNVetoLateralReflectorWithHoles, pHoleForPMT, pHoleRotation, hPosHole);
    }
  }

  G4RotationMatrix* pFoilRotation = new G4RotationMatrix();
  pFoilRotation->rotateZ(dAngleOffset);
  m_pNVetoLateralReflectorLogicalVolume = new G4LogicalVolume(pNVetoLateralReflectorWithHoles, Tyvek, "nVetoLateralReflectorLogicalVolume", 0, 0, 0);
  m_pNVetoLateralReflectorPhysicalVolume = new G4PVPlacement(pFoilRotation, G4ThreeVector(0,0,dFoilSidePanelZ),
							     "nVetoLateralReflector", m_pNVetoLateralReflectorLogicalVolume, m_pWaterPhysicalVolume, false, 0);


  //====== Top and bottom reflectors ======
  const G4double zPlane_h[]={-0.5*dFoilThickness,0.5*dFoilThickness};
  const G4double rInner_h[]={0,0};
  const G4double rOuter_h[]={dOctagonApothema-0.5*dFoilThickness,dOctagonApothema-0.5*dFoilThickness};
  G4Polyhedra *pNVetoHorizontalReflector = new G4Polyhedra("nVetoHorizontalReflector",0,2*M_PI,8,2,zPlane_h,rInner_h,rOuter_h);

  //------ Top Reflector with holes (calibration ports) ------
  G4Box *pHoleForIBeltBox = new G4Box("HoleForIBeltBox", 11.*cm, 11.*cm, 10.*mm);
  G4Tubs *pHoleForNGTubs = new G4Tubs("HoleForNGTubs", 0, 10.*cm, 10.*cm, 0, 360.*deg);
  G4RotationMatrix* pHoleRotation = new G4RotationMatrix();
  pHoleRotation->rotateZ(-dAngleOffset);
  G4SubtractionSolid* pNVetoTopReflectorWithHoles;
  pNVetoTopReflectorWithHoles = new G4SubtractionSolid("nVetoTopReflectorWithHoles", pNVetoHorizontalReflector, pHoleForIBeltBox, pHoleRotation, G4ThreeVector(906.*mm, 423.*mm, 0));
  pNVetoTopReflectorWithHoles = new G4SubtractionSolid("nVetoTopReflectorWithHoles", pNVetoTopReflectorWithHoles, pHoleForNGTubs, pHoleRotation, G4ThreeVector(835.*mm, -390.*mm, 0));
  //  pNVetoTopReflectorWithHoles = new G4SubtractionSolid("nVetoTopReflectorWithHoles", pNVetoTopReflectorWithHoles, pHoleForNGTubs, pHoleRotation, G4ThreeVector(316.*mm, 868.*mm, 0));
  pNVetoTopReflectorWithHoles = new G4SubtractionSolid("nVetoTopReflectorWithHoles", pNVetoTopReflectorWithHoles, pHoleForNGTubs, pHoleRotation, G4ThreeVector(-924.*mm, 0, 0));

  m_pNVetoTopReflectorLogicalVolume = new G4LogicalVolume(pNVetoTopReflectorWithHoles, Tyvek, "nVetoTopReflectorLogicalVolume");
  m_pNVetoTopReflectorPhysicalVolume = new G4PVPlacement(pFoilRotation, G4ThreeVector(0, 0, dFoilSidePanelZ + 0.5 * dFoilSidePanelHeight - 0.5 * dFoilThickness),
							 "nVetoTopReflector",m_pNVetoTopReflectorLogicalVolume, m_pWaterPhysicalVolume, false, 0);

  //------ Bottom Reflector ------
  m_pNVetoBottomReflectorLogicalVolume = new G4LogicalVolume(pNVetoHorizontalReflector, Tyvek, "nVetoBottomReflectorLogicalVolume");
  m_pNVetoBottomReflectorPhysicalVolume = new G4PVPlacement(pFoilRotation, G4ThreeVector(0, 0, dFoilSidePanelZ - 0.5 * dFoilSidePanelHeight + 0.5 * dFoilThickness),
                                                         "nVetoBottomReflector",m_pNVetoBottomReflectorLogicalVolume, m_pWaterPhysicalVolume, false, 0);
  //====== ePTFE Reflectivity ======
  const G4int NUM = 2;
  G4double pp[NUM] = {1*eV, 5*eV};
  G4double dReflectivity[NUM] = {0.99, 0.99};
  G4MaterialPropertiesTable *pFoilMPT = new G4MaterialPropertiesTable();
  pFoilMPT->AddProperty("REFLECTIVITY", pp, dReflectivity, NUM);

  G4OpticalSurface *pOpFoilSurface = new G4OpticalSurface("OpFoilSurface");
  pOpFoilSurface->SetType(dielectric_dielectric);
  pOpFoilSurface->SetModel(unified);
  pOpFoilSurface->SetFinish(groundfrontpainted);
  pOpFoilSurface->SetMaterialPropertiesTable(pFoilMPT);
  new G4LogicalBorderSurface("nVetoLateralReflectorOpticalSurface", m_pWaterPhysicalVolume, m_pNVetoLateralReflectorPhysicalVolume, pOpFoilSurface);
  new G4LogicalBorderSurface("nVetoTopReflectorOpticalSurface", m_pWaterPhysicalVolume, m_pNVetoTopReflectorPhysicalVolume, pOpFoilSurface);
  new G4LogicalBorderSurface("nVetoBottomReflectorOpticalSurface", m_pWaterPhysicalVolume, m_pNVetoBottomReflectorPhysicalVolume, pOpFoilSurface);

  //====== Visible attributes ======
  G4Colour hFoilColour(0.6, 0.6, 0.6, 0.1);
  G4VisAttributes *pFoilVisAtt = new G4VisAttributes(hFoilColour);
  pFoilVisAtt->SetVisibility(true);
  m_pNVetoLateralReflectorLogicalVolume->SetVisAttributes(pFoilVisAtt);
  m_pNVetoTopReflectorLogicalVolume->SetVisAttributes(pFoilVisAtt);
  m_pNVetoBottomReflectorLogicalVolume->SetVisAttributes(pFoilVisAtt);
}

void Xenon1tDetectorConstruction::VolumesHierarchy() {
  //=== Loop over all volumes and write to file list of:
  //      PhysicalVolume, LogicalVolume, MotherLogicalVolume ===
  //=== Pietro - 20180504
  //============================================================

  G4PhysicalVolumeStore *thePVStore =
      G4PhysicalVolumeStore::GetInstance();  // get all defined volumes

  G4String n_PhysicalVolumeName;
  G4String n_LogicalVolumeName;
  G4String n_MotherVolumeName;

  std::string f_name = pNTversion + "VolumesList.dat";
  std::ofstream f_volumeslist(f_name);
  G4cout << ">>> Writing list of volumes to file: " << f_name << G4endl;
  f_volumeslist << "PhysicalVolume LogicalVolume MotherLogicalVolume" << G4endl;

  // loop over all volumes in G4PhysicalVolumeStore and write to file
  for (size_t i = 1; i < thePVStore->size(); i++) {
    n_PhysicalVolumeName = (*thePVStore)[i]->GetName();
    n_LogicalVolumeName = (*thePVStore)[i]->GetLogicalVolume()->GetName();
    n_MotherVolumeName = (*thePVStore)[i]->GetMotherLogical()->GetName();
    f_volumeslist << n_PhysicalVolumeName << " " << n_LogicalVolumeName << " "
                  << n_MotherVolumeName << G4endl;
  }
  f_volumeslist.close();
}

void Xenon1tDetectorConstruction::OverlapCheck() {
  G4PhysicalVolumeStore *thePVStore = G4PhysicalVolumeStore::GetInstance();

  if (m_iVerbosityLevel >= 1) {
    G4cout << "\n******************************" << G4endl;
    G4cout << "***** CHECK FOR OVERLAPS *****" << G4endl;
    G4cout << "******************************\n" << G4endl;
  }

  if (m_iVerbosityLevel >= 1)
    G4cout << thePVStore->size() << " physical volumes are defined" << G4endl;

  G4bool overlapFlag = false;  // overlapFlag initialized to false

  for (size_t i = 0; i < thePVStore->size(); i++) {
    // Loop over every physical volume
    overlapFlag = (*thePVStore)[i]->CheckOverlaps(5000) | overlapFlag;
  }
}

void Xenon1tDetectorConstruction::PrintGeometryInformation() {
  G4cout << "\n";
  //============ ER masses ============
  const G4double dOuterReflectorMass =
     m_pOuterCryostatReflectorLogicalVolume->GetMass(false, false) / kg;
     G4cout << "Outer Reflector Logical Volume: " << dOuterReflectorMass << " kg " << G4endl;



  //========== Water tank ==========
  const G4double dWaterTankMass =
      m_pWaterTankTubLogicalVolume->GetMass(false, false) / (1000. * kg) +
      m_pTankConsLogicalVolume->GetMass(false, false) / (1000. * kg);
  G4cout << "Water Tank Mass:                " << dWaterTankMass << " ton"
         << G4endl;
  //========== Water ==========
  const G4double dWaterMass =
      m_pWaterLogicalVolume->GetMass(false, false) / (1000. * kg) +
      m_pWaterConsLogicalVolume->GetMass(false, false) / (1000. * kg);
  G4cout << "Water Mass:                     " << dWaterMass << " ton" << G4endl
         << G4endl;

  //========== Support structure ==========
  const G4double dlegfloorMass1 =
      m_pLegFloor1LogicalVolume->GetMass(false, false) / kg;
  G4cout << "FloorLeg 1:                     " << dlegfloorMass1 << " kg"
         << G4endl;
  const G4double dlegmediumMass1 =
      m_pLegMedium1LogicalVolume->GetMass(false, false) / kg;
  G4cout << "MediumLeg 1:                    " << dlegmediumMass1 << " kg"
         << G4endl;
  const G4double dlegmediumMass2 =
      m_pLegMedium2LogicalVolume->GetMass(false, false) / kg;
  G4cout << "MediumLeg 2:                    " << dlegmediumMass2 << " kg"
         << G4endl;
  const G4double dlegmediumMass3 =
      m_pLegMedium3LogicalVolume->GetMass(false, false) / kg;
  G4cout << "MediumLeg 3:                    " << dlegmediumMass3 << " kg"
         << G4endl;
  const G4double dlegmediumMass4 =
      m_pLegMedium4LogicalVolume->GetMass(false, false) / kg;
  G4cout << "MediumLeg 4:                    " << dlegmediumMass4 << " kg"
         << G4endl;
  const G4double dleghoriMass1 =
      m_pLegHorizontal1LogicalVolume->GetMass(false, false) / kg;
  G4cout << "horiLeg 1:                      " << dleghoriMass1 << " kg"
         << G4endl;
  const G4double dlegtiltconsMass1 =
      m_pLegTiltedCons1LogicalVolume->GetMass(false, false) / kg;
  G4cout << "tiltconsLeg 1:                  " << dlegtiltconsMass1 << " kg"
         << G4endl;
  const G4double dlegtiltconsMass2 =
      m_pLegTiltedCons2LogicalVolume->GetMass(false, false) / kg;
  G4cout << "tiltconsLeg 2:                  " << dlegtiltconsMass2 << " kg"
         << G4endl;
  const G4double dlegtiltconsMass3 =
      m_pLegTiltedCons3LogicalVolume->GetMass(false, false) / kg;
  G4cout << "tiltconsLeg 3:                  " << dlegtiltconsMass3 << " kg"
         << G4endl;
  const G4double dlegtiltconsMass4 =
      m_pLegTiltedCons4LogicalVolume->GetMass(false, false) / kg;
  G4cout << "tiltconsLeg 4:                  " << dlegtiltconsMass4 << " kg"
         << G4endl;
  const G4double dlegtopMass1 =
      m_pLegTopLogicalVolume1->GetMass(false, false) / kg;
  G4cout << "topMass 1:                      " << dlegtopMass1 << " kg"
         << G4endl;
  const G4double dlegtopMass2 =
      m_pLegTopLogicalVolume2->GetMass(false, false) / kg;
  G4cout << "topMass 2:                      " << dlegtopMass2 << " kg"
         << G4endl;
  const G4double dlegtopMass3 =
      m_pLegTopLogicalVolume3->GetMass(false, false) / kg;
  G4cout << "topMass 3:                      " << dlegtopMass3 << " kg"
         << G4endl;
  const G4double dlegtopMass4 =
      m_pLegTopLogicalVolume4->GetMass(false, false) / kg;
  G4cout << "topMass 4:                      " << dlegtopMass4 << " kg"
         << G4endl;
  const G4double dlegconMass1 =
      m_pLegConnection1LogicalVolume->GetMass(false, false) / kg;
  G4cout << "conMass 1:                      " << dlegconMass1 << " kg"
         << G4endl;
  const G4double dlegtiltMass1 =
      m_pLegTiltedLogicalVolume->GetMass(false, false) / kg;
  G4cout << "tiltMass 1:                     " << dlegtiltMass1 << " kg"
         << G4endl;
  const G4double dtotalmass =
      dlegfloorMass1 * 4. + dlegmediumMass1 + dlegmediumMass2 +
      dlegmediumMass3 + dlegmediumMass4 + dleghoriMass1 * 8. +
      dlegtiltconsMass1 + dlegtiltconsMass2 + dlegtiltconsMass3 +
      dlegtiltconsMass4 + dlegtopMass1 + dlegtopMass2 + dlegtopMass3 +
      dlegtopMass4 * 4. + dlegconMass1 * 4. + dlegtiltMass1 * 4.;
  G4cout << "Total mass support structure:   " << dtotalmass << " kg" << G4endl
         << G4endl;

  //========== Spreader ==========
  /*const G4double dspreader =
      m_pLegSpreaderLogicalVolume->GetMass(false, false) / kg;
  const G4double dtrianglespreader =
      m_pLegSpreaderPlatLogicalVolume->GetMass(false, false) / kg;

  G4cout << "Spreader:                                 "
         << dspreader * 3. + dtrianglespreader * 2. << " kg" << G4endl
         << G4endl;

  const G4double dtie = m_pTieRodLogicalVolume->GetMass(false, false) / kg;
  G4cout << "Tie:                                      " << dtie
         << " kg" << G4endl << G4endl;*/

  //========== Pipes ==========
  /*const G4double dcylpipe =
      m_pPipeCylinderSSLogicalVolume->GetMass(false, false) / kg;
  G4cout << "cylpipeext:                  " << dcylpipe << " kg" << G4endl
         << G4endl;
  const G4double dcylpipeinternal =
      m_pPipeCylinderInternalSSLogicalVolume->GetMass(false, false) / kg;
  G4cout << "cylpipeint:                  " << dcylpipeinternal << " kg"
         << G4endl << G4endl;
  const G4double dcylpipeinternal1 =
      m_pPipeCylinderInternalSSLogicalVolume_1->GetMass(false, false) / kg;
  G4cout << "cylpipeint1:                 " << dcylpipeinternal1 << " kg"
         << G4endl << G4endl;
  const G4double dcylpipeinternal2 =
      m_pPipeCylinderInternalSSLogicalVolume_2->GetMass(false, false) / kg;
  G4cout << "cylpipeint2:                 " << dcylpipeinternal2 << " kg"
         << G4endl << G4endl;
  const G4double dcylpipeinternal3 =
      m_pPipeCylinderInternalSSLogicalVolume_3->GetMass(false, false) / kg;
  G4cout << "cylpipeint3:                 " << dcylpipeinternal3 << " kg"
         << G4endl << G4endl;
  const G4double dcylpipeinternal4 =
      m_pPipeCylinderInternalSSLogicalVolume_4->GetMass(false, false) / kg;
  G4cout << "cylpipeint4:                 " << dcylpipeinternal4 << " kg"
         << G4endl << G4endl;
  const G4double dcylpipeinternal5 =
      m_pPipeCylinderInternalSSLogicalVolume_5->GetMass(false, false) / kg;
  G4cout << "cylpipeint5:                 " << dcylpipeinternal5 << " kg"
         << G4endl << G4endl;
  const G4double dcylpipeair =
      m_pPipeCylinderAirLogicalVolume->GetMass(false, false) / kg;
  G4cout << "cylpipeair:                  " << dcylpipeair << " kg" << G4endl
         << G4endl;
  const G4double dcylpipelow =
      m_pPipeCylinderLowSSLogicalVolume->GetMass(false, false) / kg;
  G4cout << "cylpipelowext:               " << dcylpipelow << " kg" << G4endl
         << G4endl;
  const G4double dcylpipelowinternal =
      m_pPipeCylinderInternalLowSSLogicalVolume->GetMass(false, false) / kg;
  G4cout << "cylpipelowint:               " << dcylpipelowinternal << " kg"
         << G4endl << G4endl;
  const G4double dcylpipelowinternal1 =
      m_pPipeCylinderInternalLowSSLogicalVolume_1->GetMass(false, false) / kg;
  G4cout << "cylpipelowint1:              " << dcylpipelowinternal1 << " kg"
         << G4endl << G4endl;
  const G4double dcylpipelowinternal2 =
      m_pPipeCylinderInternalLowSSLogicalVolume_2->GetMass(false, false) / kg;
  G4cout << "cylpipelowint2:              " << dcylpipelowinternal2 << " kg"
         << G4endl << G4endl;
  const G4double dcylpipelowinternal3 =
      m_pPipeCylinderInternalLowSSLogicalVolume_3->GetMass(false, false) / kg;
  G4cout << "cylpipelowint3:              " << dcylpipelowinternal3 << " kg"
         << G4endl << G4endl;
  const G4double dcylpipelowinternal4 =
      m_pPipeCylinderInternalSSLogicalVolume_4->GetMass(false, false) / kg;
  G4cout << "cylpipelowint4:              " << dcylpipelowinternal4 << " kg"
         << G4endl << G4endl;
  const G4double dcylpipelowinternal5 =
      m_pPipeCylinderInternalSSLogicalVolume_5->GetMass(false, false) / kg;
  G4cout << "cylpipelowint5:              " << dcylpipelowinternal5 << " kg"
         << G4endl << G4endl;

  const G4double dcylpipelowair =
      m_pPipeCylinderLowAirLogicalVolume->GetMass(false, false) / kg;
  G4cout << "cylpipelowair:               " << dcylpipelowair << " kg"
         << G4endl << G4endl;

  const G4double dtoruspipe =
      m_pPipeTorusSSLogicalVolume->GetMass(false, false) / kg;
  G4cout << "toruspipeext:                " << dtoruspipe << " kg" << G4endl
         << G4endl;
  const G4double dtoruspipeinternal =
      m_pPipeTorusInternalSSLogicalVolume->GetMass(false, false) / kg;
  G4cout << "toruspipeint:                " << dtoruspipeinternal << " kg"
         << G4endl << G4endl;
  const G4double dtoruspipeinternal1 =
      m_pPipeTorusInternalSSLogicalVolume_1->GetMass(false, false) / kg;
  G4cout << "toruspipeint1:               " << dtoruspipeinternal1 << " kg"
         << G4endl << G4endl;
  const G4double dtoruspipeinternal2 =
      m_pPipeTorusInternalSSLogicalVolume_2->GetMass(false, false) / kg;
  G4cout << "toruspipeint2:               " << dtoruspipeinternal2 << " kg"
         << G4endl << G4endl;
  const G4double dtoruspipeinternal3 =
      m_pPipeTorusInternalSSLogicalVolume_3->GetMass(false, false) / kg;
  G4cout << "toruspipeint3:               " << dtoruspipeinternal3 << " kg"
         << G4endl << G4endl;
  const G4double dtoruspipeinternal4 =
      m_pPipeTorusInternalSSLogicalVolume_4->GetMass(false, false) / kg;
  G4cout << "toruspipeint4:               " << dtoruspipeinternal4 << " kg"
         << G4endl << G4endl;
  const G4double dtoruspipeinternal5 =
      m_pPipeTorusInternalSSLogicalVolume_5->GetMass(false, false) / kg;
  G4cout << "toruspipeint5:               " << dtoruspipeinternal5 << " kg"
         << G4endl << G4endl;
  const G4double dtoruspipeair =
      m_pPipeTorusAirLogicalVolume->GetMass(false, false) / kg;
  G4cout << "toruspipeair:                " << dtoruspipeair << " kg"
         << G4endl << G4endl;
  const G4double dcylpipelong =
      m_pPipeCylinderTiltedLongSSLogicalVolume->GetMass(false, false) / kg;
  G4cout << "cylpipelongext:              " << dcylpipelong << " kg"
         << G4endl << G4endl;
  const G4double dcylpipelonginternal =
      m_pPipeCylinderTiltedLongInternalSSLogicalVolume->GetMass(false, false) /
      kg;
  G4cout << "cylpipelongint:              " << dcylpipelonginternal << " kg"
         << G4endl << G4endl;
  const G4double dcylpipelonginternal1 =
      m_pPipeCylinderTiltedLongInternalSSLogicalVolume_1->GetMass(false,
                                                                  false) /
      kg;
  G4cout << "cylpipelongint1:             " << dcylpipelonginternal1
         << " kg" << G4endl << G4endl;
  const G4double dcylpipelonginternal2 =
      m_pPipeCylinderTiltedLongInternalSSLogicalVolume_2->GetMass(false,
                                                                  false) /
      kg;
  G4cout << "cylpipelongint2:             " << dcylpipelonginternal2
         << " kg" << G4endl << G4endl;
  const G4double dcylpipelonginternal3 =
      m_pPipeCylinderTiltedLongInternalSSLogicalVolume_3->GetMass(false,
                                                                  false) /
      kg;
  G4cout << "cylpipelongint3:             " << dcylpipelonginternal3
         << " kg" << G4endl << G4endl;
  const G4double dcylpipelonginternal4 =
      m_pPipeCylinderTiltedLongInternalSSLogicalVolume_4->GetMass(false,
                                                                  false) /
      kg;
  G4cout << "cylpipelongint4:             " << dcylpipelonginternal4
         << " kg" << G4endl << G4endl;
  const G4double dcylpipelonginternal5 =
      m_pPipeCylinderTiltedLongInternalSSLogicalVolume_5->GetMass(false,
                                                                  false) /
      kg;
  G4cout << "cylpipelongint5:             " << dcylpipelonginternal5
         << " kg" << G4endl << G4endl;
  const G4double dcylpipelongair =
      m_pPipeCylinderTiltedLongAirLogicalVolume->GetMass(false, false) / kg;
  G4cout << "cylpipelongair:              " << dcylpipelongair << " kg"
         << G4endl << G4endl;
  const G4double flange = m_pPipeBaseLogicalVolume->GetMass(false, false) / kg;
  G4cout << "flange:                      " << flange << " kg" << G4endl <<
  G4endl;
  const G4double flangeint =
      m_pPipeBaseInternalLogicalVolume->GetMass(false, false) / kg;
  G4cout << "flangeint:                   " << flangeint << " kg" << G4endl
         << G4endl;
  const G4double flangeint1 =
      m_pPipeBaseInternal1LogicalVolume->GetMass(false, false) / kg;
  G4cout << "flangeint1:                  " << flangeint1 << " kg" << G4endl
         << G4endl;
  const G4double flangeint2 =
      m_pPipeBaseInternal2LogicalVolume->GetMass(false, false) / kg;
  G4cout << "flangeint2:                  " << flangeint2 << " kg" << G4endl
         << G4endl;
  const G4double flangeint3 =
      m_pPipeBaseInternal3LogicalVolume->GetMass(false, false) / kg;
  G4cout << "flangeint3:                  " << flangeint3 << " kg" << G4endl
         << G4endl;
  const G4double flangeint4 =
      m_pPipeBaseInternal4LogicalVolume->GetMass(false, false) / kg;
  G4cout << "flangeint4:                  " << flangeint4 << " kg" << G4endl
         << G4endl;
  const G4double flangeint5 =
      m_pPipeBaseInternal5LogicalVolume->GetMass(false, false) / kg;
  G4cout << "flangeint5:                  " << flangeint5 << " kg" << G4endl
         << G4endl;
  const G4double masscentralpipe =
      2. * flange + dcylpipe + dcylpipeinternal + dcylpipeinternal1 +
      dcylpipeinternal2 + dcylpipeinternal3 + dcylpipeinternal4 +
      dcylpipeinternal5 + dcylpipelow + dcylpipelowinternal +
      dcylpipelowinternal1 + dcylpipelowinternal2 + dcylpipelowinternal3 +
      dcylpipelowinternal4 + dcylpipelowinternal5 + dtoruspipe +
      dtoruspipeinternal + dtoruspipeinternal1 + dtoruspipeinternal2 +
      dtoruspipeinternal3 + dtoruspipeinternal4 + dtoruspipeinternal5 +
      dcylpipelong + dcylpipelonginternal + dcylpipelonginternal1 +
      dcylpipelonginternal2 + dcylpipelonginternal3 + dcylpipelonginternal4 +
      dcylpipelonginternal5;
  G4cout << "masstotal:                                " << masscentralpipe << "
  kg" << G4endl
         << G4endl;

  const G4double dcylpipesmall =
      m_pPipeCylinderSmallSSLogicalVolume->GetMass(false, false) / kg;
  G4cout << "cylpipesmall:                " << dcylpipesmall << " kg"
         << G4endl << G4endl;
  const G4double dcylpipeairsmall =
      m_pPipeCylinderAirSmallLogicalVolume->GetMass(false, false) / kg;
  G4cout << "cylpipeairsmall:             " << dcylpipeairsmall << " kg"
         << G4endl << G4endl;
  const G4double dtoruspipesmall =
      m_pPipeTorusSmallSSLogicalVolume->GetMass(false, false) / kg;
  G4cout << "toruspipesmallext:           " << dtoruspipesmall << " kg"
         << G4endl << G4endl;
  const G4double dtoruspipeairsmall =
      m_pPipeTorusAirSmallLogicalVolume->GetMass(false, false) / kg;
  G4cout << "toruspipesmallairext:        " << dtoruspipeairsmall
         << " kg" << G4endl << G4endl;
  const G4double dcylpipelongsmall =
      m_pPipeCylinderTiltedLongSmallSSLogicalVolume->GetMass(false, false) / kg;
  G4cout << "cylpipesmall:                " << dcylpipelongsmall << " kg"
         << G4endl << G4endl;
  const G4double dcylpipeairlongsmall =
      m_pPipeCylinderTiltedLongAirSmallLogicalVolume->GetMass(false, false) /
      kg;
  G4cout << "cylpipeairlongsmall:         " << dcylpipeairlongsmall
         << " kg" << G4endl << G4endl;
  const G4double dflangesmall =
      m_pPipeBaseSmallLogicalVolume->GetMass(false, false) / kg;
  G4cout << "flangesmall:                 " << dflangesmall << " kg" << G4endl
         << G4endl;
  const G4double masssmallpipe =
      dflangesmall + dcylpipesmall + dtoruspipesmall + dcylpipelongsmall;
  G4cout << "masssmalltotal:                           " << masssmallpipe << "
  kg"
         << G4endl << G4endl;
  const G4double dtolon = m_pPipeTolonLogicalVolume->GetMass(false, false) / kg;
  G4cout << "tolon:                       " << dtolon << " kg" << G4endl <<
  G4endl;*/

  //========== Veto PMTs ==========
  const G4int iNbLSPMTs = (G4int)GetGeometryParameter("NbLSPMTs");
  G4cout << "Number of LS PMTs:              " << iNbLSPMTs << G4endl;
  const G4int iNbWaterPMTs = (G4int)GetGeometryParameter("NbWaterPMTs");
  G4cout << "Number of Water PMTs:           " << iNbWaterPMTs << G4endl;
  const G4int iNbVetoPMTs = iNbLSPMTs + iNbWaterPMTs;
  G4cout << "Total Number of Veto PMTs:      " << iNbVetoPMTs << G4endl
         << G4endl;

  //========== Cryostats ==========
  dOuterCryostatMass =
      m_pOuterCryostatLogicalVolume->GetMass(false, false) / kg;
  G4cout << "Outer Cryostat Mass:            " << dOuterCryostatMass << " kg"
         << G4endl;
  dInnerCryostatMass =
      m_pInnerCryostatLogicalVolume->GetMass(false, false) / kg;
  G4cout << "Inner Cryostat Mass:            " << dInnerCryostatMass << " kg"
         << G4endl;
  dTotalCryostatMass = dOuterCryostatMass + dInnerCryostatMass;
  G4cout << "Total Cryostat Mass:            " << dTotalCryostatMass << " kg"
         << G4endl;
}

G4ThreeVector Xenon1tDetectorConstruction::GetPMTPosition(G4int iPMTNb,
                                                          PMTPart ePMTPart) {
  const G4int iNbTopPMTs = (G4int)GetGeometryParameter("NbTopPMTs");
  const G4int iNbBottomPMTs = (G4int)GetGeometryParameter("NbBottomPMTs");
  const G4int iNbLSPMTs = (G4int)GetGeometryParameter("NbLSPMTs");
  const G4int iNbWaterPMTs = (G4int)GetGeometryParameter("NbWaterPMTs");

  G4ThreeVector hPos;

  if (iPMTNb < iNbTopPMTs + iNbBottomPMTs + iNbLSPMTs)
    hPos = GetPMTPositionLSArray(iPMTNb, ePMTPart);
  else if (iPMTNb < iNbTopPMTs + iNbBottomPMTs + iNbLSPMTs + iNbWaterPMTs)
    hPos = GetPMTPositionWaterArray(iPMTNb, ePMTPart);

  // G4double PmtRadius = sqrt( pow(hPos.x(),2) + pow(hPos.y(),2) );
  // if(ePMTPart == PMT_WINDOW)
  // G4cout << "iPMTNb " << iPMTNb << " position " << hPos << " Radius " <<
  // PmtRadius <<  G4endl;

  return hPos;
}

G4ThreeVector Xenon1tDetectorConstruction::GetPMTPositionLSArray(
    G4int iPMTNb, PMTPart ePMTPart) {
  const G4int iNbTopPMTs = (G4int)GetGeometryParameter("NbTopPMTs");
  const G4int iNbBottomPMTs = (G4int)GetGeometryParameter("NbBottomPMTs");
  const G4int iNbLSTopPMTs = (G4int)GetGeometryParameter("NbLSTopPMTs");
  const G4int iNbLSBottomPMTs = (G4int)GetGeometryParameter("NbLSBottomPMTs");
  const G4int iNbLSSidePMTColumns =
      (G4int)GetGeometryParameter("NbLSSidePMTColumns");
  const G4int iNbLSSidePMTRows = (G4int)GetGeometryParameter("NbLSSidePMTRows");

  const G4double dLSTopPMTWindowZ = GetGeometryParameter("LSTopPMTWindowZ");
  const G4double dLSBottomPMTWindowZ =
      GetGeometryParameter("LSBottomPMTWindowZ");
  const G4double dLSSidePMTWindowR = GetGeometryParameter("LSSidePMTWindowR");
  const G4double dPMTBodyOffset = GetGeometryParameter("PMTWindowTopZ") +
                                  0.5 * GetGeometryParameter("PMTBodyHeight");
  const G4double dPMTBaseOffset = GetGeometryParameter("PMTWindowTopZ") +
                                  GetGeometryParameter("PMTBodyHeight") +
                                  0.5 * GetGeometryParameter("PMTBaseHeight");

  const G4double dLSTopPMTDistance = GetGeometryParameter("LSTopPMTDistance");
  const G4double dLSBottomPMTDistance =
      GetGeometryParameter("LSBottomPMTDistance");
  const G4double dLSSidePMTRowDistance =
      GetGeometryParameter("LSSidePMTRowDistance");
  //  const G4double dLSSidePMTColumnDistance = 2.*M_PI * dLSSidePMTWindowR /
  //  iNbLSSidePMTColumns;

  const G4int iNbLSTopPMTRows = 3;
  const G4int hLSTopPMTsPerRow[iNbLSTopPMTRows] = {3, 3, 3};
  const G4int iNbLSBottomPMTRows = 3;
  const G4int hLSBottomPMTsPerRow[iNbLSBottomPMTRows] = {3, 3, 3};

  const G4int iNbLSSidePMTColumnsPerPanel = iNbLSSidePMTColumns / 4;

  G4ThreeVector hPos;

  iPMTNb -= iNbTopPMTs + iNbBottomPMTs;

  if (iPMTNb < iNbLSTopPMTs) {
    switch (ePMTPart) {
      case PMT_WINDOW:
        hPos.setZ(dLSTopPMTWindowZ);
        break;

      case PMT_BODY:
        hPos.setZ(dLSTopPMTWindowZ + dPMTBodyOffset);
        break;

      case PMT_BASE:
        hPos.setZ(dLSTopPMTWindowZ + dPMTBaseOffset);
        break;

      case LXeTorlonPiece:
        break;

      case TorlonPiece:
        break;
    }

    vector<G4double> hLSTopPMTsRowOffset;
    for (G4int iLSTopPMTRowNb = 0; iLSTopPMTRowNb < iNbLSTopPMTRows;
         iLSTopPMTRowNb++)
      hLSTopPMTsRowOffset.push_back((G4double)(
          -0.5 * (hLSTopPMTsPerRow[iLSTopPMTRowNb] - 1) * dLSTopPMTDistance));

    G4int iPMTNb1 = 0;
    G4int iTotal = hLSTopPMTsPerRow[0];
    while (iPMTNb + 1 > iTotal) iTotal += hLSTopPMTsPerRow[++iPMTNb1];

    G4int iPMTNb2 = iPMTNb + hLSTopPMTsPerRow[iPMTNb1] - iTotal;

    hPos.setX(hLSTopPMTsRowOffset[iPMTNb1] + iPMTNb2 * dLSTopPMTDistance);
    hPos.setY((0.5 * (iNbLSTopPMTRows - 1) - iPMTNb1) * dLSTopPMTDistance);
  }

  else if (iPMTNb < iNbLSTopPMTs + iNbLSBottomPMTs) {
    iPMTNb -= iNbLSTopPMTs;

    switch (ePMTPart) {
      case PMT_WINDOW:
        hPos.setZ(dLSBottomPMTWindowZ);
        break;

      case PMT_BODY:
        hPos.setZ(dLSBottomPMTWindowZ - dPMTBodyOffset);
        break;

      case PMT_BASE:
        hPos.setZ(dLSBottomPMTWindowZ - dPMTBaseOffset);
        break;

      case LXeTorlonPiece:
        break;

      case TorlonPiece:
        break;
    }

    vector<G4double> hLSBottomPMTsRowOffset;
    for (G4int iLSBottomPMTRowNb = 0; iLSBottomPMTRowNb < iNbLSBottomPMTRows;
         iLSBottomPMTRowNb++)
      hLSBottomPMTsRowOffset.push_back(
          (G4double)(-0.5 * (hLSBottomPMTsPerRow[iLSBottomPMTRowNb] - 1) *
                     dLSBottomPMTDistance));

    G4int iPMTNb1 = 0;
    G4int iTotal = hLSBottomPMTsPerRow[0];
    while (iPMTNb + 1 > iTotal) iTotal += hLSBottomPMTsPerRow[++iPMTNb1];

    G4int iPMTNb2 = iPMTNb + hLSBottomPMTsPerRow[iPMTNb1] - iTotal;

    hPos.setX(hLSBottomPMTsRowOffset[iPMTNb1] + iPMTNb2 * dLSBottomPMTDistance);
    hPos.setY((0.5 * (iNbLSBottomPMTRows - 1) - iPMTNb1) *
              dLSBottomPMTDistance);
  }

  else {
    if (pnVetoConfiguration == "Cylinder") {
      iPMTNb -= iNbLSTopPMTs + iNbLSBottomPMTs;

      switch (ePMTPart) {
        case PMT_WINDOW:
          hPos.setX(dLSSidePMTWindowR);
          break;

        case PMT_BODY:
          hPos.setX(dLSSidePMTWindowR + dPMTBodyOffset);
          break;

        case PMT_BASE:
          hPos.setX(dLSSidePMTWindowR + dPMTBaseOffset);
          break;

        case LXeTorlonPiece:
          break;

        case TorlonPiece:
          break;
      }

      hPos.setY(0);
      hPos.setZ((0.5 * (iNbLSSidePMTRows - 1) - (iPMTNb / iNbLSSidePMTColumns)) *
                    dLSSidePMTRowDistance + 600 * mm);
      hPos.rotateZ((iPMTNb % iNbLSSidePMTColumns) *
          (360. / iNbLSSidePMTColumns) * deg);
    } else if (pnVetoConfiguration == "Box") {
      iPMTNb -= iNbLSTopPMTs + iNbLSBottomPMTs;

      const G4double dLSSidePMTWindowX =
        GetGeometryParameter("LSSidePMTWindowX");

      switch (ePMTPart) {
        case PMT_WINDOW:
          hPos.setX(dLSSidePMTWindowX);
          break;

        case PMT_BODY:
          hPos.setX(dLSSidePMTWindowX + dPMTBodyOffset);
          break;

        case PMT_BASE:
          hPos.setX(dLSSidePMTWindowX + dPMTBaseOffset);
          break;

        case LXeTorlonPiece:
          break;

        case TorlonPiece:
          break;
      }

      hPos.setY((iPMTNb / 4) % iNbLSSidePMTColumnsPerPanel /
          (iNbLSSidePMTColumnsPerPanel - 1.) * 3200.*mm - 1600.*mm);
      hPos.setZ((iPMTNb / 4) % iNbLSSidePMTRows / (iNbLSSidePMTRows - 1.) *
          2600.*mm - 1100.*mm);
      hPos.rotateZ((iPMTNb % 4) * 90.*deg);
    }
    else if (pnVetoConfiguration == "Octagon"){ //Pietro 190709
      iPMTNb -= iNbLSTopPMTs + iNbLSBottomPMTs;
      const G4double x_pos = GetGeometryParameter("LSSidePMTWindowX");
      const G4int rows = GetGeometryParameter("NbLSSidePMTRows"); // no. of PMT rows
      const G4double z_max = GetGeometryParameter("NVetoPMTTopRowZ"); // z position of first PMT row
      const G4double z_min = GetGeometryParameter("NVetoPMTBottomRowZ"); // z position of last PMT row
      G4int colId = 0;
      G4int rowId = 0;
      G4int sideId = 0;

      if (iPMTNb < 72){ // nVeto PMTs on the 4 sides along Nikhef support structure (3 columns per side)
	const G4double dDistanceBetweenColumns = GetGeometryParameter("DistanceBetweenColumns_SideAlongSS");
	const G4double y_max = dDistanceBetweenColumns;
	const G4double y_min = -dDistanceBetweenColumns;
	const G4int cols = 3; // no. of PMT columns
	rowId = (iPMTNb/4)/cols;
	colId = (iPMTNb/4)%cols;
	sideId = iPMTNb%4;
	switch(ePMTPart)
	  {
	  case PMT_WINDOW:
	    hPos.setX(x_pos); break;
	  case PMT_BODY:
	    hPos.setX(x_pos + dPMTBodyOffset); break;
	  case PMT_BASE:
	    hPos.setX(x_pos + dPMTBaseOffset); break;
          case LXeTorlonPiece: break;
          case TorlonPiece: break;
	  }
	hPos.setY(y_min+colId/(cols-1.)*(y_max-y_min));
	hPos.setZ(z_min+rowId/(rows-1.)*(z_max-z_min) + 900.*mm);
	hPos.rotateZ(sideId*(360./4)*deg);
      }

      else{ // nVeto PMTs on the 4 tilted sides of the octagon (2 columns per side)
	const G4double d = GetGeometryParameter("DistanceBetweenColumns_DiagonalSide") * 0.5 * mm; // displacement of columns from the center of the octagon side
	const G4int cols = 2; // no. of PMT columns
	rowId = ((iPMTNb-72)/4)/cols;
	colId = (iPMTNb/4)%cols;
	sideId = iPMTNb%4;
	G4double angle = 45.*deg + sideId*(360./4)*deg;
	G4double centerX = x_pos*cos(angle);
	G4double centerY = x_pos*sin(angle);
	switch(ePMTPart)
	  {
	  case PMT_WINDOW:
	    hPos.setX(centerX + d*cos(angle + 90.*deg + colId*180.*deg));
	    hPos.setY(centerY + d*sin(angle + 90.*deg + colId*180.*deg));
	    break;
	  case PMT_BODY:
	    hPos.setX(centerX + d*cos(angle + 90.*deg + colId*180.*deg) + dPMTBodyOffset*cos(angle));
	    hPos.setY(centerY + d*sin(angle + 90.*deg + colId*180.*deg) + dPMTBodyOffset*sin(angle));
	    break;
	  case PMT_BASE:
	    hPos.setX(centerX + d*cos(angle + 90.*deg + colId*180.*deg) + dPMTBaseOffset*cos(angle));
	    hPos.setY(centerY + d*sin(angle + 90.*deg + colId*180.*deg) + dPMTBaseOffset*sin(angle));
	    break;
          case LXeTorlonPiece: break;
          case TorlonPiece: break;
	  }
	hPos.setZ(z_min+rowId/(rows-1.)*(z_max-z_min) + 900.*mm);
      }
    }
  }
  return hPos;
}

G4ThreeVector Xenon1tDetectorConstruction::GetPMTPositionWaterArray(
    G4int iPMTNb, PMTPart ePMTPart) {
  const G4int iNbTopPMTs = (G4int)GetGeometryParameter("NbTopPMTs");
  const G4int iNbBottomPMTs = (G4int)GetGeometryParameter("NbBottomPMTs");
  const G4int iNbLSPMTs = (G4int)GetGeometryParameter("NbLSPMTs");
  const G4int iNbWaterTopPMTs = (G4int)GetGeometryParameter("NbWaterTopPMTs");
  const G4int iNbWaterBottomPMTs =
      (G4int)GetGeometryParameter("NbWaterBottomPMTs");
  const G4int iNbWaterSidePMTColumns =
      (G4int)GetGeometryParameter("NbWaterSidePMTColumns");
  const G4int iNbWaterSidePMTRows =
      (G4int)GetGeometryParameter("NbWaterSidePMTRows");

  const G4double dWaterTopPMTWindowZ =
      GetGeometryParameter("WaterTopPMTWindowZ");
  const G4double dWaterBottomPMTWindowZ =
      GetGeometryParameter("WaterBottomPMTWindowZ");
  const G4double dWaterSidePMTWindowR =
      GetGeometryParameter("WaterSidePMTWindowR");
  const G4double dPMTBodyOffset = GetGeometryParameter("PMTWindowTopZ") +
                                  0.5 * GetGeometryParameter("PMTBodyHeight");
  const G4double dPMTBaseOffset = GetGeometryParameter("PMTWindowTopZ") +
                                  GetGeometryParameter("PMTBodyHeight") +
                                  0.5 * GetGeometryParameter("PMTBaseHeight");
  const G4double dWaterSidePMTRowDistance =
      GetGeometryParameter("WaterSidePMTRowDistance");

  G4ThreeVector hPos;

  iPMTNb -= iNbTopPMTs + iNbBottomPMTs + iNbLSPMTs;

  // TOP WATER PMTs
  if (iPMTNb < iNbWaterTopPMTs) {
    switch (ePMTPart) {
      case PMT_WINDOW:
        hPos.setZ(dWaterTopPMTWindowZ);
        break;

      case PMT_BODY:
        hPos.setZ(dWaterTopPMTWindowZ + dPMTBodyOffset);
        break;

      case PMT_BASE:
        hPos.setZ(dWaterTopPMTWindowZ + dPMTBaseOffset);
        break;

      case LXeTorlonPiece:
        break;

      case TorlonPiece:
        break;
    }

    // TO BE CHANGED T1 <--> T2	TOP PMTs @top cylinder (9010)
    const G4int hWaterTopPMTsX[24] = {
        4462,  4157,  3570,  2739,  1722,  587,   -588,  -1723, -2740,
        -3571, -4158, -4462, -4462, -4158, -3571, -2740, -1723, -588,
        587,   1722,  2739,  3570,  4157,  4462};  // SERENA // final Tank R =
                                                   // 4.8m

    const G4int hWaterTopPMTsY[24] = {
        587,   1722,  2739,  3570,  4157,  4462,  4462,  4157,  3570,
        2739,  1722,  587,   -588,  -1723, -2740, -3571, -4158, -4462,
        -4462, -4158, -3571, -2740, -1723, -588};  // SERENA // final Tank R =
                                                   // 4.8m

    hPos.setX(hWaterTopPMTsX[iPMTNb]);  // SERENA
    hPos.setY(hWaterTopPMTsY[iPMTNb]);  // SERENA
  }

  // BOTTOM WATER PMTs
  else if (iPMTNb < iNbWaterTopPMTs + iNbWaterBottomPMTs) {
    iPMTNb -= iNbWaterTopPMTs;

    switch (ePMTPart) {
      case PMT_WINDOW:
        hPos.setZ(dWaterBottomPMTWindowZ);
        break;

      case PMT_BODY:
        hPos.setZ(dWaterBottomPMTWindowZ - dPMTBodyOffset);
        break;

      case PMT_BASE:
        hPos.setZ(dWaterBottomPMTWindowZ - dPMTBaseOffset);
        break;

      case LXeTorlonPiece:
        break;

      case TorlonPiece:
        break;
    }

    const G4int hWaterBottomPMTsX[24] = {
        4462,  4157,  3570,  2739,  1722,  587,   -588,  -1723, -2740,
        -3571, -4158, -4462, -4462, -4158, -3571, -2740, -1723, -588,
        587,   1722,  2739,  3570,  4157,  4462};  // SERENA // final Tank R =
                                                   // 4.8m
    const G4int hWaterBottomPMTsY[24] = {
        587,   1722,  2739,  3570,  4157,  4462,  4462,  4157,  3570,
        2739,  1722,  587,   -588,  -1723, -2740, -3571, -4158, -4462,
        -4462, -4158, -3571, -2740, -1723, -588};  // SERENA // final Tank R =
                                                   // 4.8m

    hPos.setX(hWaterBottomPMTsX[iPMTNb]);  // SERENA
    hPos.setY(hWaterBottomPMTsY[iPMTNb]);  // SERENA
  }

  // LATERAL WATER PMTs
  else {
    iPMTNb -= iNbWaterTopPMTs + iNbWaterBottomPMTs;

    switch (ePMTPart) {
      case PMT_WINDOW:
        hPos.setX(dWaterSidePMTWindowR);
        break;

      case PMT_BODY:
        hPos.setX(dWaterSidePMTWindowR + dPMTBodyOffset);
        break;

      case PMT_BASE:
        hPos.setX(dWaterSidePMTWindowR + dPMTBaseOffset);
        break;

      case LXeTorlonPiece:
        break;

      case TorlonPiece:
        break;
    }

    hPos.setY(0);
    hPos.setZ(
        (0.5 * (iNbWaterSidePMTRows - 1) - (iPMTNb / iNbWaterSidePMTColumns)) *
        dWaterSidePMTRowDistance);
    hPos.rotateZ((iPMTNb % iNbWaterSidePMTColumns) *
                     (360. / iNbWaterSidePMTColumns) * deg +
                 15. * deg);  // SERENA, to be compatible with Rainer
  }

  return hPos;
}

G4RotationMatrix *Xenon1tDetectorConstruction::GetPMTRotation(G4int iPMTNb) {
  const G4int iNbTopPMTs = (G4int)GetGeometryParameter("NbTopPMTs");
  const G4int iNbBottomPMTs = (G4int)GetGeometryParameter("NbBottomPMTs");

  const G4int iNbLSPMTs = (G4int)GetGeometryParameter("NbLSPMTs");
  const G4int iNbLSTopPMTs = (G4int)GetGeometryParameter("NbLSTopPMTs");
  const G4int iNbLSBottomPMTs = (G4int)GetGeometryParameter("NbLSBottomPMTs");
  const G4int iNbLSSidePMTColumns =
      (G4int)GetGeometryParameter("NbLSSidePMTColumns");

  const G4int iNbWaterPMTs = (G4int)GetGeometryParameter("NbWaterPMTs");
  const G4int iNbWaterTopPMTs = (G4int)GetGeometryParameter("NbWaterTopPMTs");
  const G4int iNbWaterBottomPMTs =
      (G4int)GetGeometryParameter("NbWaterBottomPMTs");
  const G4int iNbWaterSidePMTColumns =
      (G4int)GetGeometryParameter("NbWaterSidePMTColumns");

  G4RotationMatrix *pRotationMatrix = new G4RotationMatrix();

  if (iPMTNb < iNbTopPMTs)
    pRotationMatrix->rotateX(180. * deg);
  else if (iPMTNb < iNbTopPMTs + iNbBottomPMTs)
    pRotationMatrix->rotateX(0. * deg);
  else if (iPMTNb < iNbTopPMTs + iNbBottomPMTs + iNbLSPMTs) {
    if (iPMTNb < iNbTopPMTs + iNbBottomPMTs + iNbLSTopPMTs)
      pRotationMatrix->rotateX(0. * deg);
    else if (iPMTNb <
             iNbTopPMTs + iNbBottomPMTs + iNbLSTopPMTs + iNbLSBottomPMTs)
      pRotationMatrix->rotateX(180. * deg);
    else { // nVeto PMTs
      pRotationMatrix->rotateY(-90. * deg); // horizontal position
      if (pnVetoConfiguration == "Cylinder") {
        pRotationMatrix->rotateX(((iPMTNb - iNbTopPMTs - iNbBottomPMTs -
                                   iNbLSTopPMTs - iNbLSBottomPMTs) %
                                  iNbLSSidePMTColumns) *
                                 (360. / iNbLSSidePMTColumns) * deg);
      } else if (pnVetoConfiguration == "Box") {
        pRotationMatrix->rotateX(((iPMTNb % 4) * 90. + 180)*deg);
      } else if (pnVetoConfiguration == "Octagon"){ //Pietro 190709
	G4int id = iPMTNb - iNbTopPMTs - iNbBottomPMTs - iNbLSTopPMTs - iNbLSBottomPMTs;
	G4int sideId = id%4;
	G4double angle;
	if(id < 72){
	  angle = (sideId*(360./4)+0.); // sides along Nikhef support structure
	} else angle = (sideId*(360./4) + 45.); // diagonal sides of the octagon
	pRotationMatrix->rotateX(angle*deg);
      }
    }
  } else if (iPMTNb < iNbTopPMTs + iNbBottomPMTs + iNbLSPMTs +
                          iNbWaterPMTs)  // WATER PMTS
  {
    if (iPMTNb <
        iNbTopPMTs + iNbBottomPMTs + iNbLSPMTs + iNbWaterTopPMTs)  // TOP
      pRotationMatrix->rotateX(0. * deg);                          // SERENA
    else if (iPMTNb < iNbTopPMTs + iNbBottomPMTs + iNbLSPMTs + iNbWaterTopPMTs +
                          iNbWaterBottomPMTs)  // BOTTOM
      pRotationMatrix->rotateX(180. * deg);
    else  // LATERAL
    {
      pRotationMatrix->rotateY(-90. * deg);
      pRotationMatrix->rotateX(
          ((iPMTNb - iNbTopPMTs - iNbBottomPMTs - iNbLSPMTs - iNbWaterTopPMTs -
            iNbWaterBottomPMTs) %
           iNbWaterSidePMTColumns) *
              (360. / iNbWaterSidePMTColumns) * deg +
          15 * deg);  // SERENA, to be compatible with Rainer
    }
  }

  return pRotationMatrix;
}

void Xenon1tDetectorConstruction::SetLXeTeflonReflectivity(G4double dLXeReflectivity) {
  Materials->SetLXeTeflonReflectivity(dLXeReflectivity);
}

void Xenon1tDetectorConstruction::SetGXeTeflonReflectivity(G4double dGXeReflectivity) {
  Materials->SetGXeTeflonReflectivity(dGXeReflectivity);
}

void Xenon1tDetectorConstruction::SetLXeTeflonUnpolishedReflectivity(G4double dLXeReflectivity) {
  Materials->SetLXeTeflonUnpolishedReflectivity(dLXeReflectivity);
}

void Xenon1tDetectorConstruction::SetGXeTeflonUnpolishedReflectivity(G4double dGXeReflectivity) {
  Materials->SetGXeTeflonUnpolishedReflectivity(dGXeReflectivity);
}

void Xenon1tDetectorConstruction::SetRealS2Mesh(G4bool RealMesh) {
  if (RealMesh) {
    G4cout << "Xenon1tDetectorConstruction: Using real S2Mesh geometry" << G4endl;
  }
  pRealS2Mesh = RealMesh;
}

void Xenon1tDetectorConstruction::SetRealBottomScreeningMesh(G4bool RealMesh) {
  if (RealMesh) {
    G4cout << "Xenon1tDetectorConstruction: Using real BottomScreeningMesh geometry" << G4endl;
  }
  pRealBottomScreeningMesh = RealMesh;
}

void Xenon1tDetectorConstruction::SetRealCathodeMesh(G4bool RealMesh) {
  if (RealMesh) {
    G4cout << "Xenon1tDetectorConstruction: Using real CathodeMesh geometry" << G4endl;
  }
  pRealCathodeMesh = RealMesh;
}

void Xenon1tDetectorConstruction::SetRealTopScreeningMesh(G4bool RealMesh) {
  if (RealMesh) {
    G4cout << "Xenon1tDetectorConstruction: Using real TopScreeningMesh geometry" << G4endl;
  }
  pRealTopScreeningMesh = RealMesh;
}

void Xenon1tDetectorConstruction::SetRealAnodeMesh(G4bool RealMesh) {
  if (RealMesh) {
    G4cout << "Xenon1tDetectorConstruction: Using real AnodeMesh geometry" << G4endl;
  }
  pRealAnodeMesh = RealMesh;
}

void Xenon1tDetectorConstruction::SetRealGateMesh(G4bool RealMesh) {
  if (RealMesh) {
    G4cout << "Xenon1tDetectorConstruction: Using real GateMesh geometry" << G4endl;
  }
  pRealGateMesh = RealMesh;
}

void Xenon1tDetectorConstruction::SetRealS2WireDiameter(G4double WireDiameter) {
  G4cout << "Xenon1tDetectorConstruction: Setting real S2 generation mesh wire diameter to "
         << WireDiameter / mm << " mm" << G4endl;

  pRealS2MeshWireDiameter = WireDiameter;
}

void Xenon1tDetectorConstruction::SetLXeScintillation(G4bool bScintillation) {
  G4cout << "Xenon1tDetectorConstruction: Setting LXe(GXe) scintillation to " << bScintillation
         << G4endl;

  G4Material *pLXeMaterial = G4Material::GetMaterial(G4String("LXe"));
  if (pLXeMaterial) {
    G4MaterialPropertiesTable *pLXePropertiesTable =
        pLXeMaterial->GetMaterialPropertiesTable();
    if (bScintillation)
      pLXePropertiesTable->AddConstProperty("SCINTILLATIONYIELD",
                                            1000. / (1.0 * keV));
  } else {
    G4cout << "ls!> LXe materials not found!" << G4endl;
    exit(-1);
  }

  G4Material *pGXeMaterial = G4Material::GetMaterial(G4String("GXe"));
  if (pGXeMaterial) {
    G4MaterialPropertiesTable *pGXePropertiesTable =
        pGXeMaterial->GetMaterialPropertiesTable();
    if (bScintillation)
      pGXePropertiesTable->AddConstProperty("SCINTILLATIONYIELD",
                                            1000. / (1.0 * keV));
  } else {
    G4cout << "ls!> GXe materials not found!" << G4endl;
    exit(-1);
  }
}

void Xenon1tDetectorConstruction::SetLXeElectricField(G4double dElectricField) {
  // G4Material *pLXeMaterial = G4Material::GetMaterial(G4String("LXe"));
  G4cout << "Xenon1tDetectorConstruction: Setting LXe electric field to "
         << dElectricField / (kilovolt / cm) << " [kilovolt/cm]" << G4endl;

  // if(pLXeMaterial)
  //  {
  //    G4MaterialPropertiesTable *pLXePropertiesTable =
  //    pLXeMaterial->GetMaterialPropertiesTable();
  //
  //    pLXePropertiesTable->RemoveProperty("ELECTRICFIELDGATE");
  //    pLXePropertiesTable->AddConstProperty("ELECTRICFIELDGATE",dElectricField);
  //  }
  pLXeElectricField = dElectricField;
}

void Xenon1tDetectorConstruction::SetGXeElectricField(G4double dElectricField) {
  // G4Material *pGXeMaterial = G4Material::GetMaterial(G4String("GXe"));
  G4cout << "Xenon1tDetectorConstruction: Setting GXe electric field to "
         << dElectricField / (kilovolt / cm) << " [kilovolt/cm]" << G4endl;

  // if(pGXeMaterial)
  //  {
  //    G4MaterialPropertiesTable *pGXePropertiesTable =
  //    pGXeMaterial->GetMaterialPropertiesTable();
  //
  //    pGXePropertiesTable->RemoveProperty("ELECTRICFIELD");
  //    pGXePropertiesTable->AddConstProperty("ELECTRICFIELD",dElectricField);
  //  }
  pGXeElectricField = dElectricField;
}

// DR 20160620 - Activation of the scintillation for the Gd_LScint
void Xenon1tDetectorConstruction::SetGdLScintScintillation(
    G4bool bGdLScintScintillation) {
  G4cout << "Xenon1tDetectorConstruction: Setting GdLScint scintillation to " << bGdLScintScintillation
         << G4endl;
  G4Material *pGdLScintMaterial =
      G4Material::GetMaterial(G4String("Gd_LScint"));

  if (pGdLScintMaterial) {
    G4MaterialPropertiesTable *pGdLScintPropertiesTable =
        pGdLScintMaterial->GetMaterialPropertiesTable();
    if (bGdLScintScintillation)
      pGdLScintPropertiesTable->AddConstProperty("SCINTILLATIONYIELD",
                                                 9000. / (1.0 * MeV));
  } else {
    G4cout << "ls!> GdLScint materials not found!" << G4endl;
    exit(-1);
  }
}

void Xenon1tDetectorConstruction::SetLXeAbsorbtionLength(
    G4double dAbsorbtionLength) {
  G4Material *pLXeMaterial = G4Material::GetMaterial(G4String("LXe"));

  if (pLXeMaterial) {
    G4cout << "Xenon1tDetectorConstruction: Setting LXe absorbtion length to "
           << dAbsorbtionLength / cm << " cm" << G4endl;

    G4MaterialPropertiesTable *pLXePropertiesTable =
        pLXeMaterial->GetMaterialPropertiesTable();

    const G4int iNbEntries = 3;

    G4double LXe_PP[iNbEntries] = {6.91 * eV, 6.98 * eV, 7.05 * eV};
    G4double LXe_ABSL[iNbEntries] = {dAbsorbtionLength, dAbsorbtionLength,
                                     dAbsorbtionLength};
    pLXePropertiesTable->RemoveProperty("ABSLENGTH");
    pLXePropertiesTable->AddProperty("ABSLENGTH", LXe_PP, LXe_ABSL, iNbEntries);
  } else {
    G4cout << "ls!> LXe materials not found!" << G4endl;
    exit(-1);
  }
}

// Lutz
void Xenon1tDetectorConstruction::SetGXeAbsorbtionLength(
    G4double dAbsorbtionLength) {
  G4Material *pGXeMaterial = G4Material::GetMaterial(G4String("GXe"));

  if (pGXeMaterial) {
    G4cout << "Xenon1tDetectorConstruction: Setting GXe absorbtion length to " << dAbsorbtionLength / m
           << " m" << G4endl;

    G4MaterialPropertiesTable *pGXePropertiesTable =
        pGXeMaterial->GetMaterialPropertiesTable();

    const G4int iNbEntries = 3;

    G4double GXe_PP[iNbEntries] = {6.91 * eV, 6.98 * eV, 7.05 * eV};
    G4double GXe_ABSL[iNbEntries] = {dAbsorbtionLength, dAbsorbtionLength,
                                     dAbsorbtionLength};
    pGXePropertiesTable->RemoveProperty("ABSLENGTH");
    pGXePropertiesTable->AddProperty("ABSLENGTH", GXe_PP, GXe_ABSL, iNbEntries);
  } else {
    G4cout << "ls!> GXe materials not found!" << G4endl;
    exit(-1);
  }
}

// Lutz
void Xenon1tDetectorConstruction::SetLXeRefractionIndex(
    G4double dRefractionIndex) {
  G4Material *pLXeMaterial = G4Material::GetMaterial(G4String("LXe"));

  if (pLXeMaterial) {
    G4cout << "Xenon1tDetectorConstruction: Setting LXe refraction index to " << dRefractionIndex
           << " " << G4endl;

    G4MaterialPropertiesTable *pLXePropertiesTable =
        pLXeMaterial->GetMaterialPropertiesTable();

    const G4int iNbEntries = 3;

    G4double LXe_PP[iNbEntries] = {6.91 * eV, 6.98 * eV, 7.05 * eV};
    G4double LXe_RI[iNbEntries] = {dRefractionIndex, dRefractionIndex,
                                   dRefractionIndex};
    pLXePropertiesTable->RemoveProperty("RINDEX");
    pLXePropertiesTable->AddProperty("RINDEX", LXe_PP, LXe_RI, iNbEntries);
  } else {
    G4cout << "ls!> LXe materials not found!" << G4endl;
    exit(-1);
  }
}

void Xenon1tDetectorConstruction::SetLXeRayScatterLength(
    G4double dRayScatterLength) {
  G4Material *pLXeMaterial = G4Material::GetMaterial(G4String("LXe"));

  if (pLXeMaterial) {
    G4cout << "Xenon1tDetectorConstruction: Setting LXe scattering length to "
           << dRayScatterLength / cm << " cm" << G4endl;

    G4MaterialPropertiesTable *pLXePropertiesTable =
        pLXeMaterial->GetMaterialPropertiesTable();

    const G4int iNbEntries = 3;

    G4double LXe_PP[iNbEntries] = {6.91 * eV, 6.98 * eV, 7.05 * eV};
    G4double LXe_SCAT[iNbEntries] = {dRayScatterLength, dRayScatterLength,
                                     dRayScatterLength};
    pLXePropertiesTable->RemoveProperty("RAYLEIGH");
    pLXePropertiesTable->AddProperty("RAYLEIGH", LXe_PP, LXe_SCAT, iNbEntries);
  } else {
    G4cout << "ls!> LXe materials not found!" << G4endl;
    exit(-1);
  }
}

void Xenon1tDetectorConstruction::SetGridMeshTransparency(
    G4double dTransparency) {
  G4Material *pGridMeshMaterial =
      G4Material::GetMaterial(G4String("GridMeshAluminium"));

  if (pGridMeshMaterial) {
    G4cout << "Xenon1tDetectorConstruction: Setting grid mesh transparency to " << dTransparency * 100
           << " %" << G4endl;
    G4double dAbsorptionLength =
        ((G4double)GetGeometryParameter("GridMeshThickness")) /
        (-log(dTransparency));
    G4MaterialPropertiesTable *pGridMeshPropertiesTable =
        pGridMeshMaterial->GetMaterialPropertiesTable();
    const G4int iNbEntries = 3;
    G4double pdGridMeshPhotonMomentum[iNbEntries] = {6.91 * eV, 6.98 * eV,
                                                     7.05 * eV};
    G4double pdGridMeshAbsorptionLength[iNbEntries] = {
        dAbsorptionLength, dAbsorptionLength, dAbsorptionLength};
    pGridMeshPropertiesTable->RemoveProperty("ABSLENGTH");
    pGridMeshPropertiesTable->AddProperty("ABSLENGTH", pdGridMeshPhotonMomentum,
                                          pdGridMeshAbsorptionLength,
                                          iNbEntries);
  } else {
    G4cout << "ls!> grid material not found!" << G4endl;
    exit(-1);
  }
}

// Cyril, June 2014
void Xenon1tDetectorConstruction::SetTopScreeningMeshTransparency(
    G4double dTransparency) {
  G4Material *pTopScreeningMeshMaterial =
      G4Material::GetMaterial(G4String("TopScreeningMesh"));

  if (pTopScreeningMeshMaterial) {
    G4cout << "Xenon1tDetectorConstruction: Setting top screening mesh transparency to "
           << dTransparency * 100 << " %" << G4endl;
    G4double dAbsorptionLength =
        ((G4double)GetGeometryParameter("TopScreeningMeshThickness")) /
        (-log(dTransparency));
    G4MaterialPropertiesTable *pTopScreeningMeshPropertiesTable =
        pTopScreeningMeshMaterial->GetMaterialPropertiesTable();
    const G4int iNbEntries = 3;
    G4double pdTopScreeningMeshPhotonMomentum[iNbEntries] = {
        6.91 * eV, 6.98 * eV, 7.05 * eV};
    G4double pdTopScreeningMeshAbsorptionLength[iNbEntries] = {
        dAbsorptionLength, dAbsorptionLength, dAbsorptionLength};
    pTopScreeningMeshPropertiesTable->RemoveProperty("ABSLENGTH");
    pTopScreeningMeshPropertiesTable->AddProperty(
        "ABSLENGTH", pdTopScreeningMeshPhotonMomentum,
        pdTopScreeningMeshAbsorptionLength, iNbEntries);
  } else {
    G4cout << "ls!> top screenin mesh material not found!" << G4endl;
    exit(-1);
  }
}

void Xenon1tDetectorConstruction::SetBottomScreeningMeshTransparency(
    G4double dTransparency) {
  G4Material *pBottomScreeningMeshMaterial =
      G4Material::GetMaterial(G4String("BottomScreeningMesh"));

  if (pBottomScreeningMeshMaterial) {
    G4cout << "Xenon1tDetectorConstruction: Setting bottom screening mesh transparency to "
           << dTransparency * 100 << " %" << G4endl;
    G4double dAbsorptionLength =
        ((G4double)GetGeometryParameter("BottomScreeningMeshThickness")) /
        (-log(dTransparency));
    G4MaterialPropertiesTable *pBottomScreeningMeshPropertiesTable =
        pBottomScreeningMeshMaterial->GetMaterialPropertiesTable();
    const G4int iNbEntries = 3;
    G4double pdBottomScreeningMeshPhotonMomentum[iNbEntries] = {
        6.91 * eV, 6.98 * eV, 7.05 * eV};
    G4double pdBottomScreeningMeshAbsorptionLength[iNbEntries] = {
        dAbsorptionLength, dAbsorptionLength, dAbsorptionLength};
    pBottomScreeningMeshPropertiesTable->RemoveProperty("ABSLENGTH");
    pBottomScreeningMeshPropertiesTable->AddProperty(
        "ABSLENGTH", pdBottomScreeningMeshPhotonMomentum,
        pdBottomScreeningMeshAbsorptionLength, iNbEntries);
  } else {
    G4cout << "ls!> bottom screening mesh material not found!" << G4endl;
    exit(-1);
  }
}

void Xenon1tDetectorConstruction::SetCathodeMeshTransparency(
    G4double dTransparency) {
  G4Material *pCathodeMeshMaterial =
      G4Material::GetMaterial(G4String("CathodeMesh"));

  if (pCathodeMeshMaterial) {
    G4cout << "Xenon1tDetectorConstruction: Setting cathode mesh transparency to "
           << dTransparency * 100 << " %" << G4endl;
    G4double dAbsorptionLength =
        ((G4double)GetGeometryParameter("CathodeMeshThickness")) /
        (-log(dTransparency));
    G4MaterialPropertiesTable *pCathodeMeshPropertiesTable =
        pCathodeMeshMaterial->GetMaterialPropertiesTable();
    const G4int iNbEntries = 3;
    G4double pdCathodeMeshPhotonMomentum[iNbEntries] = {6.91 * eV, 6.98 * eV,
                                                        7.05 * eV};
    G4double pdCathodeMeshAbsorptionLength[iNbEntries] = {
        dAbsorptionLength, dAbsorptionLength, dAbsorptionLength};
    pCathodeMeshPropertiesTable->RemoveProperty("ABSLENGTH");
    pCathodeMeshPropertiesTable->AddProperty(
        "ABSLENGTH", pdCathodeMeshPhotonMomentum, pdCathodeMeshAbsorptionLength,
        iNbEntries);
  } else {
    G4cout << "ls!> cathode mesh material not found!" << G4endl;
    exit(-1);
  }
}

void Xenon1tDetectorConstruction::SetAnodeMeshTransparency(
    G4double dTransparency) {
  G4Material *pAnodeMeshMaterial =
      G4Material::GetMaterial(G4String("AnodeMesh"));

  if (pAnodeMeshMaterial) {
    G4cout << "Xenon1tDetectorConstruction: Setting anode mesh transparency to " << dTransparency * 100
           << " %" << G4endl;
    G4double dAbsorptionLength =
        ((G4double)GetGeometryParameter("AnodeMeshThickness")) /
        (-log(dTransparency));
    G4MaterialPropertiesTable *pAnodeMeshPropertiesTable =
        pAnodeMeshMaterial->GetMaterialPropertiesTable();
    const G4int iNbEntries = 3;
    G4double pdAnodeMeshPhotonMomentum[iNbEntries] = {6.91 * eV, 6.98 * eV,
                                                      7.05 * eV};
    G4double pdAnodeMeshAbsorptionLength[iNbEntries] = {
        dAbsorptionLength, dAbsorptionLength, dAbsorptionLength};
    pAnodeMeshPropertiesTable->RemoveProperty("ABSLENGTH");
    pAnodeMeshPropertiesTable->AddProperty(
        "ABSLENGTH", pdAnodeMeshPhotonMomentum, pdAnodeMeshAbsorptionLength,
        iNbEntries);
  } else {
    G4cout << "ls!> anode mesh material not found!" << G4endl;
    exit(-1);
  }
}

void Xenon1tDetectorConstruction::SetGateMeshTransparency(
    G4double dTransparency) {
  G4Material *pGateMeshMaterial = G4Material::GetMaterial(G4String("GateMesh"));

  if (pGateMeshMaterial) {
    G4cout << "Xenon1tDetectorConstruction: Setting gate mesh transparency to " << dTransparency * 100
           << " %" << G4endl;
    G4double dAbsorptionLength =
        ((G4double)GetGeometryParameter("GateMeshThickness")) /
        (-log(dTransparency));
    G4MaterialPropertiesTable *pGateMeshPropertiesTable =
        pGateMeshMaterial->GetMaterialPropertiesTable();
    const G4int iNbEntries = 3;
    G4double pdGateMeshPhotonMomentum[iNbEntries] = {6.91 * eV, 6.98 * eV,
                                                     7.05 * eV};
    G4double pdGateMeshAbsorptionLength[iNbEntries] = {
        dAbsorptionLength, dAbsorptionLength, dAbsorptionLength};
    pGateMeshPropertiesTable->RemoveProperty("ABSLENGTH");
    pGateMeshPropertiesTable->AddProperty("ABSLENGTH", pdGateMeshPhotonMomentum,
                                          pdGateMeshAbsorptionLength,
                                          iNbEntries);
  } else {
    G4cout << "ls!> gate mesh material not found!" << G4endl;
    exit(-1);
  }
}

// SERENA - Write the geometry parameters into a root file.
void Xenon1tDetectorConstruction::MakeDetectorPlots() {
  _fGeom = new TFile(detRootFile, "RECREATE");
  _detector = _fGeom->mkdir("detector");

  // xenon
  MakeXenonPlots();
  // cryostat
  MakeCryostatPlots();

  // TPC

  // Water tank

  // etc etc

  _fGeom->Write();
  _fGeom->Close();
}

void Xenon1tDetectorConstruction::MakeXenonPlots() {
  // make a list of materials for graphs
  G4int nmaterial = G4Material::GetNumberOfMaterials();
  if (m_iVerbosityLevel >= 1)
    G4cout << "MakeDetectorPlots:: Number of materials = " << nmaterial
           << G4endl;

  TDirectory *_materials = _detector->mkdir("materials");
  _materials->cd();

  //  for(G4int imat=0; imat<(G4int)matNames.size(); imat++){
  vector<TDirectory *> matdirs;

  for (G4int imat = 0; imat < nmaterial; imat++) {
    G4Material *mat = G4NistManager::Instance()->GetMaterial(imat);
    G4String matname = mat->GetName();
    if (m_iVerbosityLevel >= 1)
      G4cout << "MakeDetectorPlots:: " << matname << G4endl;
    G4double T = mat->GetTemperature();
    G4double rho = mat->GetDensity();
    G4double P = mat->GetPressure();

    matdirs.push_back(_materials->mkdir(matname));
    matdirs[imat]->cd();
    TParameter<double> *TemperaturePar =
        new TParameter<double>("Temperature", T);
    TemperaturePar->Write();
    TParameter<double> *DensityPar =
        new TParameter<double>("Density", rho / (g / cm3));
    DensityPar->Write();
    TParameter<double> *PressurePar =
        new TParameter<double>("Pressure", P / bar);
    PressurePar->Write();

    // G4MaterialPropertiesTable *pMaterialPropertiesTable =
    // mat->GetMaterialPropertiesTable();
    if (matname == "LXe") {
      TParameter<double> *ElectricFieldPar = new TParameter<double>(
          "ElectricField", pLXeElectricField / (kilovolt / cm));
      ElectricFieldPar->Write();
    }
    if (matname == "GXe") {
      TParameter<double> *ElectricFieldPar = new TParameter<double>(
          "ElectricField", pGXeElectricField / (kilovolt / cm));
      ElectricFieldPar->Write();
    }

    // disect the material
    size_t nele = mat->GetNumberOfElements();
    G4ElementVector *elems = (G4ElementVector *)mat->GetElementVector();
    G4double *fractionVec = (G4double *)mat->GetFractionVector();

    for (size_t iele = 0; iele < nele; iele++) {
      G4String elname = (*elems)[iele]->GetName();
      G4double frac = fractionVec[iele];
      //      G4cout <<iele<<" elem = "<<(*elems)[iele]->GetName()<<" f =
      //      "<<fractionVec[iele]<<G4endl;
      char elFrac[100];
      sprintf(elFrac, "f_%s", elname.c_str());
      TParameter<double> *_fracPar =
          new TParameter<double>((G4String)elFrac, frac);
      _fracPar->Write();
    }

    _materials->cd();
  }

  _fGeom->cd();
}

void Xenon1tDetectorConstruction::MakeCryostatPlots() {
  TDirectory *_cryostat = _detector->mkdir("cryostat");
  TDirectory *_inner = _cryostat->mkdir("inner");
  TDirectory *_outer = _cryostat->mkdir("outer");
  _cryostat->cd();

  // parameters for the outer cryostat
  _outer->cd();
  TNamed *OuterCryostatMaterialPar =
      new TNamed("OuterCryostatMaterial", pCryostatMaterial);
  OuterCryostatMaterialPar->Write();
  TParameter<double> *OuterCryostatMassPar =
      new TParameter<double>("OuterCryostatMass", dOuterCryostatMass);
  OuterCryostatMassPar->Write();
  TParameter<double> *OuterCryostatThicknessPar = new TParameter<double>(
      "OuterCryostatThickness", GetGeometryParameter("OuterCryostatThickness"));
  OuterCryostatThicknessPar->Write();
  TParameter<double> *OuterCryostatThicknessTopPar =
      new TParameter<double>("OuterCryostatThicknessTop",
                             GetGeometryParameter("OuterCryostatThicknessTop"));
  OuterCryostatThicknessTopPar->Write();
  TParameter<double> *OuterCryostatThicknessBotPar =
      new TParameter<double>("OuterCryostatThicknessBot",
                             GetGeometryParameter("OuterCryostatThicknessBot"));
  OuterCryostatThicknessBotPar->Write();
  TParameter<double> *OuterCryostatOuterDiameterPar = new TParameter<double>(
      "OuterCryostatOuterDiameter",
      GetGeometryParameter("OuterCryostatOuterDiameter"));
  OuterCryostatOuterDiameterPar->Write();
  TParameter<double> *OuterCryostatCylinderHeightPar = new TParameter<double>(
      "OuterCryostatCylinderHeight",
      GetGeometryParameter("OuterCryostatCylinderHeight"));
  OuterCryostatCylinderHeightPar->Write();
  TParameter<double> *OuterCryostatFlangeHeightPar =
      new TParameter<double>("OuterCryostatFlangeHeight",
                             GetGeometryParameter("OuterCryostatFlangeHeight"));
  OuterCryostatFlangeHeightPar->Write();
  //TParameter<double> *OuterCryostatFlangeZPar = new TParameter<double>(
  //    "OuterCryostatFlangeZ", GetGeometryParameter("OuterCryostatFlangeZ"));
  //OuterCryostatFlangeZPar->Write();
  TParameter<double> *OuterCryostatFlangeThicknessPar = new TParameter<double>(
      "OuterCryostatFlangeThickness",
      GetGeometryParameter("OuterCryostatFlangeThickness"));
  OuterCryostatFlangeThicknessPar->Write();
  TParameter<double> *OuterCryostatOffsetZPar = new TParameter<double>(
      "OuterCryostatOffsetZ", GetGeometryParameter("OuterCryostatOffsetZ"));
  OuterCryostatOffsetZPar->Write();
  TParameter<double> *OuterCryostatR0topPar = new TParameter<double>(
      "OuterCryostatR0top", GetGeometryParameter("OuterCryostatR0top"));
  OuterCryostatR0topPar->Write();
  TParameter<double> *OuterCryostatR1topPar = new TParameter<double>(
      "OuterCryostatR1top", GetGeometryParameter("OuterCryostatR1top"));
  OuterCryostatR1topPar->Write();
  TParameter<double> *OuterCryostatR0botPar = new TParameter<double>(
      "OuterCryostatR0bot", GetGeometryParameter("OuterCryostatR0bot"));
  OuterCryostatR0botPar->Write();
  TParameter<double> *OuterCryostatR1botPar = new TParameter<double>(
      "OuterCryostatR1bot", GetGeometryParameter("OuterCryostatR1bot"));
  OuterCryostatR1botPar->Write();

  // parameters for the inner cryostat
  _inner->cd();
  TNamed *InnerCryostatMaterialPar =
      new TNamed("InnerCryostatMaterial", pCryostatMaterial);
  InnerCryostatMaterialPar->Write();
  TParameter<double> *InnerCryostatMassPar =
      new TParameter<double>("InnerCryostatMass", dInnerCryostatMass);
  InnerCryostatMassPar->Write();
  TParameter<double> *InnerCryostatThicknessPar = new TParameter<double>(
      "InnerCryostatThickness", GetGeometryParameter("InnerCryostatThickness"));
  InnerCryostatThicknessPar->Write();
  TParameter<double> *InnerCryostatThicknessTopPar =
      new TParameter<double>("InnerCryostatThicknessTop",
                             GetGeometryParameter("InnerCryostatThicknessTop"));
  InnerCryostatThicknessTopPar->Write();
  TParameter<double> *InnerCryostatThicknessBotPar =
      new TParameter<double>("InnerCryostatThicknessBot",
                             GetGeometryParameter("InnerCryostatThicknessBot"));
  InnerCryostatThicknessBotPar->Write();
  TParameter<double> *InnerCryostatOuterDiameterPar = new TParameter<double>(
      "InnerCryostatOuterDiameter",
      GetGeometryParameter("InnerCryostatOuterDiameter"));
  InnerCryostatOuterDiameterPar->Write();
  TParameter<double> *InnerCryostatCylinderHeightPar = new TParameter<double>(
      "InnerCryostatCylinderHeight",
      GetGeometryParameter("InnerCryostatCylinderHeight"));
  InnerCryostatCylinderHeightPar->Write();
  TParameter<double> *InnerCryostatFlangeHeightPar =
      new TParameter<double>("InnerCryostatFlangeHeight",
                             GetGeometryParameter("InnerCryostatFlangeHeight"));
  InnerCryostatFlangeHeightPar->Write();
  //TParameter<double> *InnerCryostatFlangeZPar = new TParameter<double>(
  //    "InnerCryostatFlangeZ", GetGeometryParameter("InnerCryostatFlangeZ"));
  //InnerCryostatFlangeZPar->Write();
  TParameter<double> *InnerCryostatFlangeThicknessPar = new TParameter<double>(
      "InnerCryostatFlangeThickness",
      GetGeometryParameter("InnerCryostatFlangeThickness"));
  InnerCryostatFlangeThicknessPar->Write();
  TParameter<double> *InnerCryostatOffsetZPar = new TParameter<double>(
      "InnerCryostatOffsetZ", GetGeometryParameter("InnerCryostatOffsetZ"));
  InnerCryostatOffsetZPar->Write();
  TParameter<double> *InnerCryostatR0topPar = new TParameter<double>(
      "InnerCryostatR0top", GetGeometryParameter("InnerCryostatR0top"));
  InnerCryostatR0topPar->Write();
  TParameter<double> *InnerCryostatR1topPar = new TParameter<double>(
      "InnerCryostatR1top", GetGeometryParameter("InnerCryostatR1top"));
  InnerCryostatR1topPar->Write();
  TParameter<double> *InnerCryostatR0botPar = new TParameter<double>(
      "InnerCryostatR0bot", GetGeometryParameter("InnerCryostatR0bot"));
  InnerCryostatR0botPar->Write();
  TParameter<double> *InnerCryostatR1botPar = new TParameter<double>(
      "InnerCryostatR1bot", GetGeometryParameter("InnerCryostatR1bot"));
  InnerCryostatR1botPar->Write();

  _fGeom->cd();
}
