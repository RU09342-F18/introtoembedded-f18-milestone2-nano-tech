/*
 * Functions.c
 *
 *  Created on: Nov 16, 2018
 *      Author: Tyler
 */

float Convert_VtoR(float vout){               // funtion for converting the vout value to resistance
  float R2_value;

  R2_value = (vout * 10000) / (vout - 3.3);

  return R2_value;
}

float Convert_RtoT(float R2_value){             // // function for converting the resistance value to temperature
  float resist;
  float temperature;
  resist = R2_value;

  if ((resist <= 32554) & (resist > 19872)){
    temperature = - 0.0008 * resist + 25.311;
  }
  else if ((resist <= 19872) & (resist > 10000)){
    temperature = - 0.0015 * resist + 39.319;
  }
  else if ((resist <= 10000) & (resist > 4372)){
    temperature = - 0.0035 * resist + 59.061;
  }
  else if ((resist <= 4372) & (resist > 1753)){
    temperature = - 0.0094 * resist + 84.646;
  }
  else if ((resist <= 1753) & (resist > 786)){
    temperature = - 0.0256 * resist + 113.46;
  }
  else if ((resist <= 786) & (resist > 442.6)){
    temperature = - 0.0579 * resist + 139.74;
  }
  else if ((resist <= 442.6) & (resist > 182.6)){
    temperature = - 0.1334 * resist + 171.64;
  }
  return temperature;
}

