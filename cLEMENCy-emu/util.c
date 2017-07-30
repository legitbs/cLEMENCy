//
//  util.c
//  cLEMENCy util functions
//
//  Created by Lightning on 2017/07/20.
//  Copyright (c) 2017 Lightning. All rights reserved.
//

int IsNum(char *Data)
{
	//make sure all the data is a hex value
	while(*Data)
	{
		if((*Data < '0') || (*Data > '9'))
			return 0;
		Data++;
	}

	return 1;
}

int IsHex(char *Data)
{
	//make sure all the data is a hex value
	while(*Data)
	{
		if(*Data < '0')
			return 0;
		else if((*Data > '9') && (*Data < 'A'))
			return 0;
		else if((*Data > 'F') && (*Data < 'a'))
			return 0;
		else if(*Data > 'f')
			return 0;
		Data++;
	}

	return 1;
}
