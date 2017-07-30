//
//  clemency_nfo.h
//  cLEMENCy NFO header
//
//  Created by Lightning on 2017/07/11.
//  Copyright (c) 2017 Lightning. All rights reserved.
//

#ifndef CLEMENCY_NFO
#define CLEMENCY_NFO

void SetupClemencyNFO();
unsigned short ClemencyNFO_Read9(unsigned int Location);
unsigned int ClemencyNFO_Protection(unsigned int Location, unsigned int Protection);

#endif
