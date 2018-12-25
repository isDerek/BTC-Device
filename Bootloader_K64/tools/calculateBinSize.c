int calculateBinSize(char* binStartAddress, int checkTotalSize)
{
		unsigned int binTotalSize;
		int CODE_SIZE = 0x20000;
		for(binTotalSize = 0 ; binTotalSize < CODE_SIZE ; binTotalSize ++)
	{
		int checkSize = 0 , validSize = 0;
		if(*(binStartAddress+binTotalSize) == 0xFF )
		{		
			for(checkSize = 0; checkSize<checkTotalSize;checkSize++)
			{
				if(*(binStartAddress+binTotalSize+checkSize) == 0xFF)
				{
					validSize++;
				}
			}		
		}
		if(validSize == checkTotalSize)
		{
//			printf("binTotalSize = %d\n\r",binTotalSize);
			return binTotalSize;
		}
	}		
		return 0;	
}
