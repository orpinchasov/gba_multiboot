uint8_t reverseBits(uint8_t num) 
{ 
    uint8_t NO_OF_BITS = 8;
    uint8_t reverse_num = 0;
    int i; 
    
    for (i = 0; i < NO_OF_BITS; i++) 
    { 
        if((num & (1 << i))) 
           reverse_num |= 1 << ((NO_OF_BITS - 1) - i);   
   } 
    return reverse_num; 
} 
