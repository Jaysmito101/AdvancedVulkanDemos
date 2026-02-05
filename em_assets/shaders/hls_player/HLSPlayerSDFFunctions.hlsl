#ifndef HLSL_PLAYER_SDF_FUNCTIONS_HLSL
#define HLSL_PLAYER_SDF_FUNCTIONS_HLSL

#include "MathUtils"
#include "SDFUtils"
#include "ShaderAdapter"

static const float3 screenPositions[4] = {
    float3(-20.0, 12.0, -8.0),
    float3( 20.0, 12.0, -5.0),
    float3(-18.0, 12.0,  15.0),
    float3( 22.0, 12.0,  18.0)
};
    
static const float screenRotations[4] = { 0.2, -0.25, 0.15, -0.18 };


float3 sdRetroTV(float3 p, int tvIndex) {
    p = p / 10.0;
    float legHeight = 0.5;  
    float bodyBottom = -0.45; 
    
    // main body
    float3 pBody = p - float3(0.0, 0.1, -0.15);
    float dBody = sdRoundBox(pBody, float3(0.75, 0.55, 0.35), 0.08);
    
    // screen
    float3 pScreen = p - float3(0.0, 0.1, 0.28);
    float dScreen = sdBox(pScreen, float3(0.55, 0.40, 0.02));
    
    // housing
    float3 pBezel = p - float3(0.0, 0.1, 0.20);
    float dBezel = sdRoundBox(pBezel, float3(0.62, 0.47, 0.04), 0.02);
    dBezel = max(dBezel, -sdBox(p - float3(0.0, 0.1, 0.30), float3(0.54, 0.39, 0.15)));
    
    // speaker gril
    float3 pSpeaker = p - float3(-0.68, 0.1, 0.1);
    float dSpeaker = sdRoundBox(pSpeaker, float3(0.04, 0.35, 0.08), 0.02);
    
    // control panel on right side
    float3 pControls = p - float3(0.68, 0.1, 0.15);
    float dControls = sdRoundBox(pControls, float3(0.04, 0.40, 0.06), 0.01);
    
    // knobs on control panel
    float3 pKnob1 = p - float3(0.72, 0.3, 0.22);
    float dKnob1 = sdSphere(pKnob1, 0.04);
    float3 pKnob2 = p - float3(0.72, 0.0, 0.22);
    float dKnob2 = sdSphere(pKnob2, 0.035);
    float3 pKnob3 = p - float3(0.72, -0.2, 0.22);
    float dKnob3 = sdSphere(pKnob3, 0.03);
    
    float legRadius = 0.05;
    float legSpreadX = 0.55;
    float legSpreadZ = 0.22;
    float legTop = bodyBottom;
    float legBottom = -1.2;
    float actualLegHeight = legTop - legBottom;
    
    // front left leg 
    float3 pLeg1 = p - float3(-legSpreadX, (legTop + legBottom) * 0.5, legSpreadZ);
    float dLeg1 = sdCappedCylinder(pLeg1, actualLegHeight * 0.5, legRadius);
    
    // front right leg
    float3 pLeg2 = p - float3(legSpreadX, (legTop + legBottom) * 0.5, legSpreadZ);
    float dLeg2 = sdCappedCylinder(pLeg2, actualLegHeight * 0.5, legRadius);
    
    // back left leg
    float3 pLeg3 = p - float3(-legSpreadX, (legTop + legBottom) * 0.5, -legSpreadZ);
    float dLeg3 = sdCappedCylinder(pLeg3, actualLegHeight * 0.5, legRadius);
    
    // back right leg
    float3 pLeg4 = p - float3(legSpreadX, (legTop + legBottom) * 0.5, -legSpreadZ);
    float dLeg4 = sdCappedCylinder(pLeg4, actualLegHeight * 0.5, legRadius);
    
    float dLegs = min(min(dLeg1, dLeg2), min(dLeg3, dLeg4));
    
    // antenna base on top
    float3 pAntennaBase = p - float3(0.0, 0.68, -0.1);
    float dAntennaBase = sdCappedCylinder(pAntennaBase, 0.02, 0.08);
    
    // antennas
    float3 pAnt1 = p - float3(-0.15, 0.90, -0.1);
    float dAnt1 = sdCapsule(p, float3(0.0, 0.68, -0.1), float3(-0.25, 1.1, -0.08), 0.012);
    float dAnt2 = sdCapsule(p, float3(0.0, 0.68, -0.1), float3(0.25, 1.1, -0.08), 0.012);
    float dAntennas = min(dAnt1, dAnt2);
    
    float d = dBody;
    float mat = 3.0;

    // screen glass 
    if (dScreen < d) {
        d = dScreen;
        mat = 10.0 + (float)tvIndex; // screen material with TV index
    }
    
    // bezel frame - dark wood or plastic
    if (dBezel < d) {
        d = dBezel;
        mat = 5.0; 
    }
    
    // speaker grill
    if (dSpeaker < d) {
        d = dSpeaker;
        mat = 8.0; 
    }
    
    // control panel
    if (dControls < d) {
        d = dControls;
        mat = 5.0; 
    }
    
    // knobs
    float dKnobs = min(min(dKnob1, dKnob2), dKnob3);
    if (dKnobs < d) {
        d = dKnobs;
        mat = 9.0;
    }
    
    // wooden legs
    if (dLegs < d) {
        d = dLegs;
        mat = 4.0; 
    }
    
    // antenna base and ears
    if (dAntennaBase < d) {
        d = dAntennaBase;
        mat = 6.0; 
    }
    if (dAntennas < d) {
        d = dAntennas;
        mat = 9.0; 
    }
    
    return float3(d * 10.0, mat, (float)tvIndex);
}


float3 mapScene(float3 p) {
    float floorDist = p.y;
    float3 result = float3(floorDist, 1.0, -1.0);
    
    for (int i = 0; i < 4; i++) {
        float3 localP = p - screenPositions[i];
        localP.xz = rotateXZ(localP.xz, screenRotations[i]);
        
        float3 tvResult = sdRetroTV(localP, i);
        if (tvResult.x < result.x) {
            result = tvResult;
        }
    }
    
    return result;
}



#endif // HLSL_PLAYER_SDF_FUNCTIONS_HLSL