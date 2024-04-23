
#include "TBitGrid.h"


typedef union TBitGridChannelPtr {
            uint8_t     *b8;
            uint16_t    *b16;
            uint32_t    *b32;
            uint64_t    *b64;
} TBitGridChannelPtr;

//

typedef struct {
    TBitGridIterator        base;
    unsigned int            channelIdx;
} TBitGridIterator_1C;

typedef struct {
    TBitGridIterator        base;
    TCell                   channelMask;
} TBitGridIterator_NC;

//

bool
__TBitGridIteratorNext_8b_1C(
    TBitGridIterator    *iterator,
    TGridPos            *outP,
    TCell               *value
)
{
#define ITERATOR    ((TBitGridIterator_1C*)iterator)

    if ( iterator->j < iterator->dimensions.h ) {
        if ( iterator->isStarted ) {
            iterator->i++;
            if ( iterator->i >= iterator->dimensions.w ) {
                iterator->i = 0;
                iterator->j++;
                
                //  We just incremented out of our grid dimensions, all done!
                if ( iterator->j >= iterator->dimensions.h ) return false;
                
                iterator->grid[ITERATOR->channelIdx].b8++;
            } else if ( (iterator->i % 8) == 0 ) {
                iterator->grid[ITERATOR->channelIdx].b8++;
            }   
        } else {
            iterator->isStarted = true;
        }
        *value = (*iterator->grid[ITERATOR->channelIdx].b8 & (1 << (iterator->i % 8))) ? (1 << ITERATOR->channelIdx) : 0;
        outP->i = iterator->i, outP->j = iterator->j;
        return true;
    }
    return false;
#undef ITERATOR
}

bool
__TBitGridIteratorNextFullRow_8b_1C(
    TBitGridIterator    *iterator,
    unsigned int        *outJ
)
{
#define ITERATOR    ((TBitGridIterator_1C*)iterator)

    while ( iterator->j < iterator->dimensions.h ) {
        unsigned int    nFullWords;
        
        if ( iterator->isStarted ) {
            //  Did we increment out of our grid dimensions?
            if ( ++iterator->j >= iterator->dimensions.h ) return false;
        } else {
            iterator->isStarted = true;
        }
        
        // Examine all whole words in the row:
        nFullWords = iterator->nFullWords;
        while ( nFullWords ) {
            if ( *iterator->grid[ITERATOR->channelIdx].b8++ != 0xFF ) break;
            nFullWords--;
        }
        if ( nFullWords == 0 ) {
            if ( iterator->nPartialBits == 0 ) {
                *outJ = iterator->j;
                return true;
            } else {
                // Examine any partial word:
                uint8_t         mask = (1 << iterator->nPartialBits) - 1;
            
                if ( (*iterator->grid[ITERATOR->channelIdx].b8++ & mask) == mask ) {
                    *outJ = iterator->j;
                    return true;
                }
            }
        } else {
            iterator->grid[ITERATOR->channelIdx].b8 += nFullWords + (iterator->nPartialBits != 0);
        }
    }
    return false;
#undef ITERATOR
}

bool
__TBitGridIteratorNext_8b_NC(
    TBitGridIterator    *iterator,
    TGridPos            *outP,
    TCell               *value
)
{
#define ITERATOR    ((TBitGridIterator_NC*)iterator)

    if ( iterator->j < iterator->dimensions.h ) {
        TCell       channelMask, channelIdx, localValue;
        bool        incPtrs = false;
        
        if ( iterator->isStarted ) {
            iterator->i++;
            if ( iterator->i >= iterator->dimensions.w ) {
                iterator->i = 0;
                iterator->j++;
                
                //  We just incremented out of our grid dimensions, all done!
                if ( iterator->j >= iterator->dimensions.h ) return false;
                incPtrs = true;
            } else {
                incPtrs = ( (iterator->i % 8) == 0 );
            }
            if ( incPtrs ) {
                channelIdx = 0, channelMask = ITERATOR->channelMask;
                while ( channelMask ) {
                    if ( channelMask & 0x1 )
                        iterator->grid[channelIdx].b8++;
                    channelIdx++, channelMask >>= 1;
                }
            }  
        } else {
            iterator->isStarted = true;
        }
        localValue = 0, channelIdx = iterator->dimensions.nChannels - 1, channelMask = 1 << (iterator->dimensions.nChannels - 1);
        while ( channelMask ) {
            if ( (channelMask & ITERATOR->channelMask) &&
                 (*iterator->grid[channelIdx].b8 & (1 << (iterator->i % 8))) )
            {
                localValue |= channelMask;
            }
            channelIdx--;
            channelMask >>= 1;
        }
        *value = localValue;
        outP->i = iterator->i, outP->j = iterator->j;
        return true;
    }
    return false;
#undef ITERATOR
}

bool
__TBitGridIteratorNextFullRow_8b_NC(
    TBitGridIterator    *iterator,
    unsigned int        *outJ
)
{
#define ITERATOR    ((TBitGridIterator_NC*)iterator)

    while ( iterator->j < iterator->dimensions.h ) {
        unsigned int    nFullWords, channelIdx, channelMask;
        
        if ( iterator->isStarted ) {
            //  Did we increment out of our grid dimensions?
            if ( ++iterator->j >= iterator->dimensions.h ) return false;
        } else {
            iterator->isStarted = true;
        }
        
        // Examine all whole words in the row:
        nFullWords = iterator->nFullWords;
        while ( nFullWords ) {
            uint8_t     combined = 0xFF;
            
            channelMask = ITERATOR->channelMask;
            channelIdx = 0;
            while ( channelMask ) {
                if ( (1 << channelIdx) & channelMask ) {
                    combined &= *iterator->grid[channelIdx].b8++;
                    channelMask &= ~(1 << channelIdx);
                }
                channelIdx++;
            }
            if ( combined != 0xFF ) break;
            nFullWords--;
        }
        if ( nFullWords == 0 ) {
            if ( iterator->nPartialBits == 0 ) {
                *outJ = iterator->j;
                return true;
            } else {
                // Examine any partial word:
                uint8_t         mask = (1 << iterator->nPartialBits) - 1;
                uint8_t         combined = 0xFF & mask;
            
                channelMask = ITERATOR->channelMask;
                channelIdx = 0;
                while ( channelMask ) {
                    if ( (1 << channelIdx) & channelMask ) {
                        combined &= *iterator->grid[channelIdx].b8++;
                        channelMask &= ~(1 << channelIdx);
                    }
                    channelIdx++;
                }
                if ( combined == mask ) {
                    *outJ = iterator->j;
                    return true;
                }
            }
        } else {
            channelMask = ITERATOR->channelMask;
            channelIdx = 0;
            while ( channelMask ) {
                if ( (1 << channelIdx) & channelMask ) {
                    iterator->grid[channelIdx].b8 += nFullWords + (iterator->nPartialBits != 0);
                    channelMask &= ~(1 << channelIdx);
                }
                channelIdx++;
            }
            
        }
    }
    return false;
#undef ITERATOR
}

//

bool
__TBitGridIteratorNext_16b_1C(
    TBitGridIterator    *iterator,
    TGridPos            *outP,
    TCell               *value
)
{
#define ITERATOR    ((TBitGridIterator_1C*)iterator)

    if ( iterator->j < iterator->dimensions.h ) {
        if ( iterator->isStarted ) {
            iterator->i++;
            if ( iterator->i >= iterator->dimensions.w ) {
                iterator->i = 0;
                iterator->j++;
                
                //  We just incremented out of our grid dimensions, all done!
                if ( iterator->j >= iterator->dimensions.h ) return false;
                
                iterator->grid[ITERATOR->channelIdx].b16++;
            } else if ( (iterator->i % 16) == 0 ) {
                iterator->grid[ITERATOR->channelIdx].b16++;
            }   
        } else {
            iterator->isStarted = true;
        }
        *value = (*iterator->grid[ITERATOR->channelIdx].b16 & (1 << (iterator->i % 16))) ? (1 << ITERATOR->channelIdx) : 0;
        outP->i = iterator->i, outP->j = iterator->j;
        return true;
    }
    return false;
#undef ITERATOR
}

bool
__TBitGridIteratorNextFullRow_16b_1C(
    TBitGridIterator    *iterator,
    unsigned int        *outJ
)
{
#define ITERATOR    ((TBitGridIterator_1C*)iterator)

    while ( iterator->j < iterator->dimensions.h ) {
        unsigned int    nFullWords;
        
        if ( iterator->isStarted ) {
            //  Did we increment out of our grid dimensions?
            if ( ++iterator->j >= iterator->dimensions.h ) return false;
        } else {
            iterator->isStarted = true;
        }
        
        // Examine all whole words in the row:
        nFullWords = iterator->nFullWords;
        while ( nFullWords ) {
            if ( *iterator->grid[ITERATOR->channelIdx].b16++ != 0xFFFF ) break;
            nFullWords--;
        }
        if ( nFullWords == 0 ) {
            if ( iterator->nPartialBits == 0 ) {
                *outJ = iterator->j;
                return true;
            } else {
                // Examine any partial word:
                uint16_t        mask = (1 << iterator->nPartialBits) - 1;
            
                if ( (*iterator->grid[ITERATOR->channelIdx].b16++ & mask) == mask ) {
                    *outJ = iterator->j;
                    return true;
                }
            }
        } else {
            iterator->grid[ITERATOR->channelIdx].b16 += nFullWords + (iterator->nPartialBits != 0);
        }
    }
    return false;
#undef ITERATOR
}

bool
__TBitGridIteratorNext_16b_NC(
    TBitGridIterator    *iterator,
    TGridPos            *outP,
    TCell               *value
)
{
#define ITERATOR    ((TBitGridIterator_NC*)iterator)

    if ( iterator->j < iterator->dimensions.h ) {
        TCell       channelMask, channelIdx, localValue;
        bool        incPtrs = false;
        
        if ( iterator->isStarted ) {
            iterator->i++;
            if ( iterator->i >= iterator->dimensions.w ) {
                iterator->i = 0;
                iterator->j++;
                
                //  We just incremented out of our grid dimensions, all done!
                if ( iterator->j >= iterator->dimensions.h ) return false;
                incPtrs = true;
            } else {
                incPtrs = ( (iterator->i % 16) == 0 );
            }
            if ( incPtrs ) {
                channelIdx = 0, channelMask = ITERATOR->channelMask;
                while ( channelMask ) {
                    if ( channelMask & 0x1 )
                        iterator->grid[channelIdx].b16++;
                    channelIdx++, channelMask >>= 1;
                }
            }  
        } else {
            iterator->isStarted = true;
        }
        localValue = 0, channelIdx = iterator->dimensions.nChannels - 1, channelMask = 1 << (iterator->dimensions.nChannels - 1);
        while ( channelMask ) {
            if ( (channelMask & ITERATOR->channelMask) &&
                 (*iterator->grid[channelIdx].b16 & (1 << (iterator->i % 16))) )
            {
                localValue |= channelMask;
            }
            channelIdx--;
            channelMask >>= 1;
        }
        *value = localValue;
        outP->i = iterator->i, outP->j = iterator->j;
        return true;
    }
    return false;
#undef ITERATOR
}

bool
__TBitGridIteratorNextFullRow_16b_NC(
    TBitGridIterator    *iterator,
    unsigned int        *outJ
)
{
#define ITERATOR    ((TBitGridIterator_NC*)iterator)

    while ( iterator->j < iterator->dimensions.h ) {
        unsigned int    nFullWords, channelIdx, channelMask;
        
        if ( iterator->isStarted ) {
            //  Did we increment out of our grid dimensions?
            if ( ++iterator->j >= iterator->dimensions.h ) return false;
        } else {
            iterator->isStarted = true;
        }
        
        // Examine all whole words in the row:
        nFullWords = iterator->nFullWords;
        while ( nFullWords ) {
            uint16_t    combined = 0xFFFF;
            
            channelMask = ITERATOR->channelMask;
            channelIdx = 0;
            while ( channelMask ) {
                if ( (1 << channelIdx) & channelMask ) {
                    combined &= *iterator->grid[channelIdx].b16++;
                    channelMask &= ~(1 << channelIdx);
                }
                channelIdx++;
            }
            if ( combined != 0xFFFF ) break;
            nFullWords--;
        }
        if ( nFullWords == 0 ) {
            if ( iterator->nPartialBits == 0 ) {
                *outJ = iterator->j;
                return true;
            } else {
                // Examine any partial word:
                uint16_t        mask = (1 << iterator->nPartialBits) - 1;
                uint16_t        combined = 0xFFFF & mask;
            
                channelMask = ITERATOR->channelMask;
                channelIdx = 0;
                while ( channelMask ) {
                    if ( (1 << channelIdx) & channelMask ) {
                        combined &= *iterator->grid[channelIdx].b16++;
                        channelMask &= ~(1 << channelIdx);
                    }
                    channelIdx++;
                }
                if ( combined == mask ) {
                    *outJ = iterator->j;
                    return true;
                }
            }
        } else {
            channelMask = ITERATOR->channelMask;
            channelIdx = 0;
            while ( channelMask ) {
                if ( (1 << channelIdx) & channelMask ) {
                    iterator->grid[channelIdx].b16 += nFullWords + (iterator->nPartialBits != 0);
                    channelMask &= ~(1 << channelIdx);
                }
                channelIdx++;
            }
            
        }
    }
    return false;
#undef ITERATOR
}

//

bool
__TBitGridIteratorNext_32b_1C(
    TBitGridIterator    *iterator,
    TGridPos            *outP,
    TCell               *value
)
{
#define ITERATOR    ((TBitGridIterator_1C*)iterator)

    if ( iterator->j < iterator->dimensions.h ) {
        if ( iterator->isStarted ) {
            iterator->i++;
            if ( iterator->i >= iterator->dimensions.w ) {
                iterator->i = 0;
                iterator->j++;
                
                //  We just incremented out of our grid dimensions, all done!
                if ( iterator->j >= iterator->dimensions.h ) return false;
                
                iterator->grid[ITERATOR->channelIdx].b32++;
            } else if ( (iterator->i % 32) == 0 ) {
                iterator->grid[ITERATOR->channelIdx].b32++;
            }   
        } else {
            iterator->isStarted = true;
        }
        *value = (*iterator->grid[ITERATOR->channelIdx].b32 & (1 << (iterator->i % 32))) ? (1 << ITERATOR->channelIdx) : 0;
        outP->i = iterator->i, outP->j = iterator->j;
        return true;
    }
    return false;
#undef ITERATOR
}

bool
__TBitGridIteratorNextFullRow_32b_1C(
    TBitGridIterator    *iterator,
    unsigned int        *outJ
)
{
#define ITERATOR    ((TBitGridIterator_1C*)iterator)

    while ( iterator->j < iterator->dimensions.h ) {
        unsigned int    nFullWords;
        
        if ( iterator->isStarted ) {
            //  Did we increment out of our grid dimensions?
            if ( ++iterator->j >= iterator->dimensions.h ) return false;
        } else {
            iterator->isStarted = true;
        }
        
        // Examine all whole words in the row:
        nFullWords = iterator->nFullWords;
        while ( nFullWords ) {
            if ( *iterator->grid[ITERATOR->channelIdx].b32++ != 0xFFFFFFFF ) break;
            nFullWords--;
        }
        if ( nFullWords == 0 ) {
            if ( iterator->nPartialBits == 0 ) {
                *outJ = iterator->j;
                return true;
            } else {
                // Examine any partial word:
                uint32_t        mask = (1 << iterator->nPartialBits) - 1;
            
                if ( (*iterator->grid[ITERATOR->channelIdx].b32++ & mask) == mask ) {
                    *outJ = iterator->j;
                    return true;
                }
            }
        } else {
            iterator->grid[ITERATOR->channelIdx].b32 += nFullWords + (iterator->nPartialBits != 0);
        }
    }
    return false;
#undef ITERATOR
}

bool
__TBitGridIteratorNext_32b_NC(
    TBitGridIterator    *iterator,
    TGridPos            *outP,
    TCell               *value
)
{
#define ITERATOR    ((TBitGridIterator_NC*)iterator)

    if ( iterator->j < iterator->dimensions.h ) {
        TCell       channelMask, channelIdx, localValue;
        bool        incPtrs = false;
        
        if ( iterator->isStarted ) {
            iterator->i++;
            if ( iterator->i >= iterator->dimensions.w ) {
                iterator->i = 0;
                iterator->j++;
                
                //  We just incremented out of our grid dimensions, all done!
                if ( iterator->j >= iterator->dimensions.h ) return false;
                incPtrs = true;
            } else {
                incPtrs = ( (iterator->i % 32) == 0 );
            }
            if ( incPtrs ) {
                channelIdx = 0, channelMask = ITERATOR->channelMask;
                while ( channelMask ) {
                    if ( channelMask & 0x1 )
                        iterator->grid[channelIdx].b32++;
                    channelIdx++, channelMask >>= 1;
                }
            }  
        } else {
            iterator->isStarted = true;
        }
        localValue = 0, channelIdx = iterator->dimensions.nChannels - 1, channelMask = 1 << (iterator->dimensions.nChannels - 1);
        while ( channelMask ) {
            if ( (channelMask & ITERATOR->channelMask) &&
                 (*iterator->grid[channelIdx].b32 & (1 << (iterator->i % 32))) )
            {
                localValue |= channelMask;
            }
            channelIdx--;
            channelMask >>= 1;
        }
        outP->i = iterator->i, outP->j = iterator->j;
        *value = localValue;
        return true;
    }
    return false;
#undef ITERATOR
}

bool
__TBitGridIteratorNextFullRow_32b_NC(
    TBitGridIterator    *iterator,
    unsigned int        *outJ
)
{
#define ITERATOR    ((TBitGridIterator_NC*)iterator)

    while ( iterator->j < iterator->dimensions.h ) {
        unsigned int    nFullWords, channelIdx, channelMask;
        
        if ( iterator->isStarted ) {
            //  Did we increment out of our grid dimensions?
            if ( ++iterator->j >= iterator->dimensions.h ) return false;
        } else {
            iterator->isStarted = true;
        }
        
        // Examine all whole words in the row:
        nFullWords = iterator->nFullWords;
        while ( nFullWords ) {
            uint32_t    combined = 0xFFFFFFFF;
            
            channelMask = ITERATOR->channelMask;
            channelIdx = 0;
            while ( channelMask ) {
                if ( (1 << channelIdx) & channelMask ) {
                    combined &= *iterator->grid[channelIdx].b32++;
                    channelMask &= ~(1 << channelIdx);
                }
                channelIdx++;
            }
            if ( combined != 0xFFFFFFFF ) break;
            nFullWords--;
        }
        if ( nFullWords == 0 ) {
            if ( iterator->nPartialBits == 0 ) {
                *outJ = iterator->j;
                return true;
            } else {
                // Examine any partial word:
                uint32_t        mask = (1 << iterator->nPartialBits) - 1;
                uint32_t        combined = 0xFFFFFFFF & mask;
            
                channelMask = ITERATOR->channelMask;
                channelIdx = 0;
                while ( channelMask ) {
                    if ( (1 << channelIdx) & channelMask ) {
                        combined &= *iterator->grid[channelIdx].b32++;
                        channelMask &= ~(1 << channelIdx);
                    }
                    channelIdx++;
                }
                if ( combined == mask ) {
                    *outJ = iterator->j;
                    return true;
                }
            }
        } else {
            channelMask = ITERATOR->channelMask;
            channelIdx = 0;
            while ( channelMask ) {
                if ( (1 << channelIdx) & channelMask ) {
                    iterator->grid[channelIdx].b32 += nFullWords + (iterator->nPartialBits != 0);
                    channelMask &= ~(1 << channelIdx);
                }
                channelIdx++;
            }
            
        }
    }
    return false;
#undef ITERATOR
}

//

bool
__TBitGridIteratorNext_64b_1C(
    TBitGridIterator    *iterator,
    TGridPos            *outP,
    TCell               *value
)
{
#define ITERATOR    ((TBitGridIterator_1C*)iterator)

    if ( iterator->j < iterator->dimensions.h ) {
        if ( iterator->isStarted ) {
            iterator->i++;
            if ( iterator->i >= iterator->dimensions.w ) {
                iterator->i = 0;
                iterator->j++;
                
                //  We just incremented out of our grid dimensions, all done!
                if ( iterator->j >= iterator->dimensions.h ) return false;
                
                iterator->grid[ITERATOR->channelIdx].b64++;
            } else if ( (iterator->i % 64) == 0 ) {
                iterator->grid[ITERATOR->channelIdx].b64++;
            }   
        } else {
            iterator->isStarted = true;
        }
        *value = (*iterator->grid[ITERATOR->channelIdx].b64 & (1 << (iterator->i % 64))) ? (1 << ITERATOR->channelIdx) : 0;
        outP->i = iterator->i, outP->j = iterator->j;
        return true;
    }
    return false;
#undef ITERATOR
}

bool
__TBitGridIteratorNextFullRow_64b_1C(
    TBitGridIterator    *iterator,
    unsigned int        *outJ
)
{
#define ITERATOR    ((TBitGridIterator_1C*)iterator)

    while ( iterator->j < iterator->dimensions.h ) {
        unsigned int    nFullWords;
        
        if ( iterator->isStarted ) {
            //  Did we increment out of our grid dimensions?
            if ( ++iterator->j >= iterator->dimensions.h ) return false;
        } else {
            iterator->isStarted = true;
        }
        
        // Examine all whole words in the row:
        nFullWords = iterator->nFullWords;
        while ( nFullWords ) {
            if ( *iterator->grid[ITERATOR->channelIdx].b64++ != 0xFFFFFFFFFFFFFFFF ) break;
            nFullWords--;
        }
        if ( nFullWords == 0 ) {
            if ( iterator->nPartialBits == 0 ) {
                *outJ = iterator->j;
                return true;
            } else {
                // Examine any partial word:
                uint64_t        mask = (1 << iterator->nPartialBits) - 1;
            
                if ( (*iterator->grid[ITERATOR->channelIdx].b64++ & mask) == mask ) {
                    *outJ = iterator->j;
                    return true;
                }
            }
        } else {
            iterator->grid[ITERATOR->channelIdx].b64 += nFullWords + (iterator->nPartialBits != 0);
        }
    }
    return false;
#undef ITERATOR
}

bool
__TBitGridIteratorNext_64b_NC(
    TBitGridIterator    *iterator,
    TGridPos            *outP,
    TCell               *value
)
{
#define ITERATOR    ((TBitGridIterator_NC*)iterator)

    if ( iterator->j < iterator->dimensions.h ) {
        TCell       channelMask, channelIdx, localValue;
        bool        incPtrs = false;
        
        if ( iterator->isStarted ) {
            iterator->i++;
            if ( iterator->i >= iterator->dimensions.w ) {
                iterator->i = 0;
                iterator->j++;
                
                //  We just incremented out of our grid dimensions, all done!
                if ( iterator->j >= iterator->dimensions.h ) return false;
                incPtrs = true;
            } else {
                incPtrs = ( (iterator->i % 64) == 0 );
            }
            if ( incPtrs ) {
                channelIdx = 0, channelMask = ITERATOR->channelMask;
                while ( channelMask ) {
                    if ( channelMask & 0x1 )
                        iterator->grid[channelIdx].b64++;
                    channelIdx++, channelMask >>= 1;
                }
            }  
        } else {
            iterator->isStarted = true;
        }
        localValue = 0, channelIdx = iterator->dimensions.nChannels - 1, channelMask = 1 << (iterator->dimensions.nChannels - 1);
        while ( channelMask ) {
            if ( (channelMask & ITERATOR->channelMask) &&
                 (*iterator->grid[channelIdx].b64 & (1 << (iterator->i % 64))) )
            {
                localValue |= channelMask;
            }
            channelIdx--;
            channelMask >>= 1;
        }
        *value = localValue;
        outP->i = iterator->i, outP->j = iterator->j;
        return true;
    }
    return false;
#undef ITERATOR
}

bool
__TBitGridIteratorNextFullRow_64b_NC(
    TBitGridIterator    *iterator,
    unsigned int        *outJ
)
{
#define ITERATOR    ((TBitGridIterator_NC*)iterator)

    while ( iterator->j < iterator->dimensions.h ) {
        unsigned int    nFullWords, channelIdx, channelMask;
        
        if ( iterator->isStarted ) {
            //  Did we increment out of our grid dimensions?
            if ( ++iterator->j >= iterator->dimensions.h ) return false;
        } else {
            iterator->isStarted = true;
        }
        
        // Examine all whole words in the row:
        nFullWords = iterator->nFullWords;
        while ( nFullWords ) {
            uint64_t    combined = 0xFFFFFFFFFFFFFFFF;
            
            channelMask = ITERATOR->channelMask;
            channelIdx = 0;
            while ( channelMask ) {
                if ( (1 << channelIdx) & channelMask ) {
                    combined &= *iterator->grid[channelIdx].b64++;
                    channelMask &= ~(1 << channelIdx);
                }
                channelIdx++;
            }
            if ( combined != 0xFFFFFFFFFFFFFFFF ) break;
            nFullWords--;
        }
        if ( nFullWords == 0 ) {
            if ( iterator->nPartialBits == 0 ) {
                *outJ = iterator->j;
                return true;
            } else {
                // Examine any partial word:
                uint64_t        mask = (1 << iterator->nPartialBits) - 1;
                uint64_t        combined = 0xFFFFFFFFFFFFFFFF & mask;
            
                channelMask = ITERATOR->channelMask;
                channelIdx = 0;
                while ( channelMask ) {
                    if ( (1 << channelIdx) & channelMask ) {
                        combined &= *iterator->grid[channelIdx].b64++;
                        channelMask &= ~(1 << channelIdx);
                    }
                    channelIdx++;
                }
                if ( combined == mask ) {
                    *outJ = iterator->j;
                    return true;
                }
            }
        } else {
            channelMask = ITERATOR->channelMask;
            channelIdx = 0;
            while ( channelMask ) {
                if ( (1 << channelIdx) & channelMask ) {
                    iterator->grid[channelIdx].b64 += nFullWords + (iterator->nPartialBits != 0);
                    channelMask &= ~(1 << channelIdx);
                }
                channelIdx++;
            }
            
        }
    }
    return false;
#undef ITERATOR
}

//
////
//

TCell  
__TBitGridGetCellValueAtIndex_8b_1C(
    TBitGrid        *bitGrid,
    TGridIndex      I
)
{
    return ((bitGrid->grid[0].b8[I.W]) & (1 << I.b)) != 0;
}
TCell  
__TBitGridGetCellValueAtIndex_8b_2C(
    TBitGrid        *bitGrid,
    TGridIndex      I
)
{
    return ((((bitGrid->grid[1].b8[I.W]) & (1 << I.b)) != 0) << 1) |
            (((bitGrid->grid[0].b8[I.W]) & (1 << I.b)) != 0);
}
TCell  
__TBitGridGetCellValueAtIndex_8b_NC(
    TBitGrid        *bitGrid,
    TGridIndex      I
)
{
    TCell           outValue = 0;
    unsigned int    c = bitGrid->dimensions.nChannels;
    
    while ( c-- )
        outValue = (outValue << 1) | (((bitGrid->grid[c].b8[I.W]) & (1 << I.b)) != 0);
    return outValue;
}

void
__TBitGridSetCellValueAtIndex_8b_1C(
    TBitGrid        *bitGrid,
    TGridIndex      I,
    TCell           value
)
{
    if ( value )
        bitGrid->grid[0].b8[I.W] |= (1 << I.b);
    else
        bitGrid->grid[0].b8[I.W] &= ~(1 << I.b);
}
void
__TBitGridSetCellValueAtIndex_8b_2C(
    TBitGrid        *bitGrid,
    TGridIndex      I,
    TCell           value
)
{
    if ( value & 0x1 )
        bitGrid->grid[0].b8[I.W] |= (1 << I.b);
    else
        bitGrid->grid[0].b8[I.W] &= ~(1 << I.b);
    if ( value & 0x2 )
        bitGrid->grid[1].b8[I.W] |= (1 << I.b);
    else
        bitGrid->grid[1].b8[I.W] &= ~(1 << I.b);
}
void
__TBitGridSetCellValueAtIndex_8b_NC(
    TBitGrid        *bitGrid,
    TGridIndex      I,
    TCell           value
)
{
    unsigned int    c = 0;
    
    while ( c < bitGrid->dimensions.nChannels ) {
        if ( value & 0x1 )
            bitGrid->grid[c].b8[I.W] |= (1 << I.b);
        else
            bitGrid->grid[c].b8[I.W] &= ~(1 << I.b);
        value >>= 1;
        c++;
    }
}

//

TCell  
__TBitGridGetCellValueAtIndex_16b_1C(
    TBitGrid        *bitGrid,
    TGridIndex      I
)
{
    return ((bitGrid->grid[0].b16[I.W]) & (1 << I.b)) != 0;
}
TCell  
__TBitGridGetCellValueAtIndex_16b_2C(
    TBitGrid        *bitGrid,
    TGridIndex      I
)
{
    return ((((bitGrid->grid[1].b16[I.W]) & (1 << I.b)) != 0) << 1) |
            (((bitGrid->grid[0].b16[I.W]) & (1 << I.b)) != 0);
}
TCell  
__TBitGridGetCellValueAtIndex_16b_NC(
    TBitGrid        *bitGrid,
    TGridIndex      I
)
{
    TCell           outValue = 0;
    unsigned int    c = bitGrid->dimensions.nChannels;
    
    while ( c-- )
        outValue = (outValue << 1) | (((bitGrid->grid[c].b16[I.W]) & (1 << I.b)) != 0);
    return outValue;
}

void
__TBitGridSetCellValueAtIndex_16b_1C(
    TBitGrid        *bitGrid,
    TGridIndex      I,
    TCell           value
)
{
    if ( value )
        bitGrid->grid[0].b16[I.W] |= (1 << I.b);
    else
        bitGrid->grid[0].b16[I.W] &= ~(1 << I.b);
}
void
__TBitGridSetCellValueAtIndex_16b_2C(
    TBitGrid        *bitGrid,
    TGridIndex      I,
    TCell           value
)
{
    if ( value & 0x1 )
        bitGrid->grid[0].b16[I.W] |= (1 << I.b);
    else
        bitGrid->grid[0].b16[I.W] &= ~(1 << I.b);
    if ( value & 0x2 )
        bitGrid->grid[1].b16[I.W] |= (1 << I.b);
    else
        bitGrid->grid[1].b16[I.W] &= ~(1 << I.b);
}
void
__TBitGridSetCellValueAtIndex_16b_NC(
    TBitGrid        *bitGrid,
    TGridIndex      I,
    TCell           value
)
{
    unsigned int    c = 0;
    
    while ( c < bitGrid->dimensions.nChannels ) {
        if ( value & 0x1 )
            bitGrid->grid[c].b16[I.W] |= (1 << I.b);
        else
            bitGrid->grid[c].b16[I.W] &= ~(1 << I.b);
        value >>= 1;
        c++;
    }
}

//

TCell  
__TBitGridGetCellValueAtIndex_32b_1C(
    TBitGrid        *bitGrid,
    TGridIndex      I
)
{
    return ((bitGrid->grid[0].b32[I.W]) & (1 << I.b)) != 0;
}
TCell  
__TBitGridGetCellValueAtIndex_32b_2C(
    TBitGrid        *bitGrid,
    TGridIndex      I
)
{
    return ((((bitGrid->grid[1].b32[I.W]) & (1 << I.b)) != 0) << 1) |
            (((bitGrid->grid[0].b32[I.W]) & (1 << I.b)) != 0);
}
TCell  
__TBitGridGetCellValueAtIndex_32b_NC(
    TBitGrid        *bitGrid,
    TGridIndex      I
)
{
    TCell           outValue = 0;
    unsigned int    c = bitGrid->dimensions.nChannels;
    
    while ( c-- )
        outValue = (outValue << 1) | (((bitGrid->grid[c].b32[I.W]) & (1 << I.b)) != 0);
    return outValue;
}

void
__TBitGridSetCellValueAtIndex_32b_1C(
    TBitGrid        *bitGrid,
    TGridIndex      I,
    TCell           value
)
{
    if ( value )
        bitGrid->grid[0].b32[I.W] |= (1 << I.b);
    else
        bitGrid->grid[0].b32[I.W] &= ~(1 << I.b);
}
void
__TBitGridSetCellValueAtIndex_32b_2C(
    TBitGrid        *bitGrid,
    TGridIndex      I,
    TCell           value
)
{
    if ( value & 0x1 )
        bitGrid->grid[0].b32[I.W] |= (1 << I.b);
    else
        bitGrid->grid[0].b32[I.W] &= ~(1 << I.b);
    if ( value & 0x2 )
        bitGrid->grid[1].b32[I.W] |= (1 << I.b);
    else
        bitGrid->grid[1].b32[I.W] &= ~(1 << I.b);
}
void
__TBitGridSetCellValueAtIndex_32b_NC(
    TBitGrid        *bitGrid,
    TGridIndex      I,
    TCell           value
)
{
    unsigned int    c = 0;
    
    while ( c < bitGrid->dimensions.nChannels ) {
        if ( value & 0x1 )
            bitGrid->grid[c].b32[I.W] |= (1 << I.b);
        else
            bitGrid->grid[c].b32[I.W] &= ~(1 << I.b);
        value >>= 1;
        c++;
    }
}

//

TCell  
__TBitGridGetCellValueAtIndex_64b_1C(
    TBitGrid        *bitGrid,
    TGridIndex      I
)
{
    return ((bitGrid->grid[0].b64[I.W]) & (1 << I.b)) != 0;
}
TCell  
__TBitGridGetCellValueAtIndex_64b_2C(
    TBitGrid        *bitGrid,
    TGridIndex      I
)
{
    return ((((bitGrid->grid[1].b64[I.W]) & (1 << I.b)) != 0) << 1) |
            (((bitGrid->grid[0].b64[I.W]) & (1 << I.b)) != 0);
}
TCell  
__TBitGridGetCellValueAtIndex_64b_NC(
    TBitGrid        *bitGrid,
    TGridIndex      I
)
{
    TCell           outValue = 0;
    unsigned int    c = bitGrid->dimensions.nChannels;
    
    while ( c-- )
        outValue = (outValue << 1) | (((bitGrid->grid[c].b64[I.W]) & (1 << I.b)) != 0);
    return outValue;
}

void
__TBitGridSetCellValueAtIndex_64b_1C(
    TBitGrid        *bitGrid,
    TGridIndex      I,
    TCell           value
)
{
    if ( value )
        bitGrid->grid[0].b64[I.W] |= (1 << I.b);
    else
        bitGrid->grid[0].b64[I.W] &= ~(1 << I.b);
}
void
__TBitGridSetCellValueAtIndex_64b_2C(
    TBitGrid        *bitGrid,
    TGridIndex      I,
    TCell           value
)
{
    if ( value & 0x1 )
        bitGrid->grid[0].b64[I.W] |= (1 << I.b);
    else
        bitGrid->grid[0].b64[I.W] &= ~(1 << I.b);
    if ( value & 0x2 )
        bitGrid->grid[1].b64[I.W] |= (1 << I.b);
    else
        bitGrid->grid[1].b64[I.W] &= ~(1 << I.b);
}
void
__TBitGridSetCellValueAtIndex_64b_NC(
    TBitGrid        *bitGrid,
    TGridIndex      I,
    TCell           value
)
{
    unsigned int    c = 0;
    
    while ( c < bitGrid->dimensions.nChannels ) {
        if ( value & 0x1 )
            bitGrid->grid[c].b64[I.W] |= (1 << I.b);
        else
            bitGrid->grid[c].b64[I.W] &= ~(1 << I.b);
        value >>= 1;
        c++;
    }
}

//

TBitGrid*
TBitGridCreate(
    TBitGridWordSize    wordSize,
    unsigned int        nChannels,
    unsigned int        w,
    unsigned int        h
)
{
    TBitGrid        *newBitGrid = NULL;
    unsigned int    nBitsPerWord, nWordsTotal, nWordsPerRow;
    size_t          channelBytes, gridBytes;
    
    if ( nChannels > 8 || nChannels < 1 ) return NULL;
    
    if ( (w < 8) || (h < 12) ) return NULL;
    
    switch ( wordSize ) {
        case TBitGridWordSizeForce8Bit:
            nBitsPerWord = 8;
            nWordsPerRow = (w + 7) / 8;
            break;
        case TBitGridWordSizeForce16Bit:
            nBitsPerWord = 16;
            nWordsPerRow = (w + 15) / 16;
            break;
        case TBitGridWordSizeForce32Bit:
            nBitsPerWord = 32;
            nWordsPerRow = (w + 31) / 32;
            break;
        case TBitGridWordSizeForce64Bit:
            nBitsPerWord = 64;
            nWordsPerRow = (w + 63) / 64;
            break;
            
        default:
        case TBitGridWordSizeDefault: {
            //
            // Determine the optimal word size for the grid width:
            //
            switch ( (w - 1) / 8 ) {
                case 0:
                    nBitsPerWord = 8;
                    nWordsPerRow = 1;
#ifdef TBITGRID_DEBUG
                    printf("Defaulting to 8-bit word\n");
#endif
                    break;
                case 1:
                    nBitsPerWord = 16;
                    nWordsPerRow = 1;
#ifdef TBITGRID_DEBUG
                    printf("Defaulting to 16-bit word\n");
#endif
                    break;
                case 3:
                    nBitsPerWord = 32;
                    nWordsPerRow = 1;
#ifdef TBITGRID_DEBUG
                    printf("Defaulting to 32-bit word\n");
#endif
                    break;
                case 4:
                case 5:
                case 6:
                case 7:
                    nBitsPerWord = 64;
                    nWordsPerRow = 1;
#ifdef TBITGRID_DEBUG
                    printf("Defaulting to 64-bit word\n");
#endif
                    break;
                default: {
                    struct {
                        uint8_t         nBitsPerWord;
                        unsigned int    nWord;
                        unsigned int    nExtraBits;
                    } swap, byWordSize[4] = {
                                                {  8,  (w + 7) / 8,  (((w + 7) / 8) << 3) - w },
                                                { 16, (w + 15) / 16, (((w + 15) / 16) << 4) - w },
                                                { 32, (w + 31) / 32, (((w + 31) / 32) << 5) - w },
                                                { 64, (w + 63) / 64, (((w + 63) / 64) << 6) - w },
                                            };
#define COMPARE_AND_SWAP(I,J) \
                    if ( (byWordSize[I].nExtraBits > byWordSize[J].nExtraBits) || \
                         ((byWordSize[I].nExtraBits == byWordSize[J].nExtraBits) && \
                          (byWordSize[I].nBitsPerWord < byWordSize[J].nBitsPerWord)) ) \
                    { \
                        swap = byWordSize[I]; byWordSize[I] = byWordSize[J]; byWordSize[J] = swap; \
                    }
                    COMPARE_AND_SWAP(0,1);
                    COMPARE_AND_SWAP(2,3);
                    COMPARE_AND_SWAP(0,2);
                    COMPARE_AND_SWAP(1,3);
                    COMPARE_AND_SWAP(1,2);
#undef COMPARE_AND_SWAP
#ifdef TBITGRID_DEBUG
                    printf("%2hhu %4u %4u\n", byWordSize[0].nBitsPerWord, byWordSize[0].nWord, byWordSize[0].nExtraBits);
                    printf("%2hhu %4u %4u\n", byWordSize[1].nBitsPerWord, byWordSize[1].nWord, byWordSize[1].nExtraBits);
                    printf("%2hhu %4u %4u\n", byWordSize[2].nBitsPerWord, byWordSize[2].nWord, byWordSize[2].nExtraBits);
                    printf("%2hhu %4u %4u\n", byWordSize[3].nBitsPerWord, byWordSize[3].nWord, byWordSize[3].nExtraBits);
#endif
                    nBitsPerWord = byWordSize[0].nBitsPerWord;
                    nWordsPerRow = byWordSize[0].nWord;
                    break;
                }
            }
            break;
        }
    }
    nWordsTotal = nWordsPerRow * h;
    
    channelBytes = nWordsTotal * (nBitsPerWord / 8);
    gridBytes = nChannels * sizeof(TBitGridChannelPtr);

    newBitGrid = (TBitGrid*)malloc(sizeof(TBitGrid) + gridBytes + nChannels * channelBytes);
    if ( newBitGrid ) {
        void            *p = (void*)newBitGrid + sizeof(TBitGrid);
        unsigned int    c;
        
        newBitGrid->dimensions.nChannels = nChannels;
        newBitGrid->dimensions.w = w;
        newBitGrid->dimensions.h = h;
        newBitGrid->dimensions.nBitsPerWord = nBitsPerWord;
        newBitGrid->dimensions.nBytesPerWord = nBitsPerWord / 8;
        newBitGrid->dimensions.nWordsTotal = nWordsTotal;
        newBitGrid->dimensions.nWordsPerRow = nWordsPerRow;
        
        newBitGrid->grid = (TBitGridStorage)p; p += gridBytes;
        
        // Clear all channel memory:
        memset(p, 0x00, nChannels * channelBytes);
        
        // Initialize each channel's storage pointer:
        c = 0;
        while ( c < nChannels ) {
            newBitGrid->grid[c++].b8 = (uint8_t*)p;
            p += channelBytes;
        }
        
        // Set the callbacks:
        switch ( nBitsPerWord ) {
            case 8:
                switch ( nChannels ) {
                    case 1:
                        newBitGrid->callbacks.getCellValueAtIndex = __TBitGridGetCellValueAtIndex_8b_1C;
                        newBitGrid->callbacks.setCellValueAtIndex = __TBitGridSetCellValueAtIndex_8b_1C;
                        break;
                    case 2:
                        newBitGrid->callbacks.getCellValueAtIndex = __TBitGridGetCellValueAtIndex_8b_2C;
                        newBitGrid->callbacks.setCellValueAtIndex = __TBitGridSetCellValueAtIndex_8b_2C;
                        break;
                    default:
                        newBitGrid->callbacks.getCellValueAtIndex = __TBitGridGetCellValueAtIndex_8b_NC;
                        newBitGrid->callbacks.setCellValueAtIndex = __TBitGridSetCellValueAtIndex_8b_NC;
                        break;
                }
                break;
            case 16:
                switch ( nChannels ) {
                    case 1:
                        newBitGrid->callbacks.getCellValueAtIndex = __TBitGridGetCellValueAtIndex_16b_1C;
                        newBitGrid->callbacks.setCellValueAtIndex = __TBitGridSetCellValueAtIndex_16b_1C;
                        break;
                    case 2:
                        newBitGrid->callbacks.getCellValueAtIndex = __TBitGridGetCellValueAtIndex_16b_2C;
                        newBitGrid->callbacks.setCellValueAtIndex = __TBitGridSetCellValueAtIndex_16b_2C;
                        break;
                    default:
                        newBitGrid->callbacks.getCellValueAtIndex = __TBitGridGetCellValueAtIndex_16b_NC;
                        newBitGrid->callbacks.setCellValueAtIndex = __TBitGridSetCellValueAtIndex_16b_NC;
                        break;
                }
                break;
            case 32:
                switch ( nChannels ) {
                    case 1:
                        newBitGrid->callbacks.getCellValueAtIndex = __TBitGridGetCellValueAtIndex_32b_1C;
                        newBitGrid->callbacks.setCellValueAtIndex = __TBitGridSetCellValueAtIndex_32b_1C;
                        break;
                    case 2:
                        newBitGrid->callbacks.getCellValueAtIndex = __TBitGridGetCellValueAtIndex_32b_2C;
                        newBitGrid->callbacks.setCellValueAtIndex = __TBitGridSetCellValueAtIndex_32b_2C;
                        break;
                    default:
                        newBitGrid->callbacks.getCellValueAtIndex = __TBitGridGetCellValueAtIndex_32b_NC;
                        newBitGrid->callbacks.setCellValueAtIndex = __TBitGridSetCellValueAtIndex_32b_NC;
                        break;
                }
                break;
            case 64:
                switch ( nChannels ) {
                    case 1:
                        newBitGrid->callbacks.getCellValueAtIndex = __TBitGridGetCellValueAtIndex_64b_1C;
                        newBitGrid->callbacks.setCellValueAtIndex = __TBitGridSetCellValueAtIndex_64b_1C;
                        break;
                    case 2:
                        newBitGrid->callbacks.getCellValueAtIndex = __TBitGridGetCellValueAtIndex_64b_2C;
                        newBitGrid->callbacks.setCellValueAtIndex = __TBitGridSetCellValueAtIndex_64b_2C;
                        break;
                    default:
                        newBitGrid->callbacks.getCellValueAtIndex = __TBitGridGetCellValueAtIndex_64b_NC;
                        newBitGrid->callbacks.setCellValueAtIndex = __TBitGridSetCellValueAtIndex_64b_NC;
                        break;
                }
                break;
        }
#ifdef TBITGRID_DEBUG
    printf("(w,h) = (%u,%u), nBitsPerWord = %u, nWords = %u, nWordsPerRow = %u, nBytesPerWord = %u\n",
            newBitGrid->dimensions.w, newBitGrid->dimensions.h,
            newBitGrid->dimensions.nBitsPerWord, newBitGrid->dimensions.nWordsTotal,
            newBitGrid->dimensions.nWordsPerRow, newBitGrid->dimensions.nBytesPerWord
        );
    printf("nChannels = %u, channelBytes = %zu, gridBytes = %zu\n", nChannels, channelBytes, gridBytes);
#endif
    }
    return newBitGrid;
}

//

void
TBitGridDestroy(
    TBitGrid*   bitGrid
)
{
    free((void*)bitGrid);
}

//

void
TBitGridFill(
    TBitGrid    *bitGrid,
    TCell       value
)
{
    unsigned int    channelIdx = 0, channelMask = 1;
    
    while ( channelIdx < bitGrid->dimensions.nChannels ) {
        if ( value & channelMask )
            memset(bitGrid->grid[channelIdx].b8, 0xFF, bitGrid->dimensions.nWordsTotal * bitGrid->dimensions.nBytesPerWord);
        else
            memset(bitGrid->grid[channelIdx].b8, 0x00, bitGrid->dimensions.nWordsTotal * bitGrid->dimensions.nBytesPerWord);
        channelIdx++;
        channelMask <<= 1;
    }
}

//

void
TBitGridScroll(
    TBitGrid        *bitGrid
)
{
    size_t          nBytesMove = (bitGrid->dimensions.nWordsTotal - bitGrid->dimensions.nWordsPerRow) * bitGrid->dimensions.nBytesPerWord;
    size_t          nBytesSet = bitGrid->dimensions.nWordsPerRow * bitGrid->dimensions.nBytesPerWord;
    unsigned int    channelIdx = bitGrid->dimensions.nChannels;
    
    while ( channelIdx-- ) {
        void        *src = (void*)bitGrid->grid[channelIdx].b8, *dst = src + bitGrid->dimensions.nWordsPerRow * bitGrid->dimensions.nBytesPerWord;
        memmove(dst, src, nBytesMove);
        memset(src, 0, nBytesSet);
    }
}

//

void
TBitGridClearLines(
    TBitGrid        *bitGrid,
    unsigned int    jLow,
    unsigned int    jHigh
)
{
    if ( jHigh < jLow ) return;
    
    if ( jLow >= bitGrid->dimensions.h ) jLow = bitGrid->dimensions.h - 1;
    if ( jHigh >= bitGrid->dimensions.h ) jHigh = bitGrid->dimensions.h - 1;
    
    // Is it the entire bitGrid?
    if ( (jLow == 0) && (jHigh >= bitGrid->dimensions.h - 1) ) {
        unsigned int    channelIdx = bitGrid->dimensions.nChannels;
        
        while ( channelIdx-- )
            memset((void*)bitGrid->grid[channelIdx].b8, 0, bitGrid->dimensions.nWordsTotal * bitGrid->dimensions.nBytesPerWord);
    } else {
        if ( jLow == 0 ) {
            // The region starts at the top row and extends down through the
            // jHigh row.  It's just a memset:
            unsigned int    channelIdx = bitGrid->dimensions.nChannels;
        
            while ( channelIdx-- )
                memset((void*)bitGrid->grid[channelIdx].b8, 0, bitGrid->dimensions.nWordsPerRow * bitGrid->dimensions.nBytesPerWord * (jHigh - jLow + 1));
        } else {
            unsigned int    channelIdx = bitGrid->dimensions.nChannels;
        
            while ( channelIdx-- ) {
                void        *dst = (void*)bitGrid->grid[channelIdx].b8 + (bitGrid->dimensions.nWordsPerRow * bitGrid->dimensions.nBytesPerWord * ((jHigh + 1) - jLow));
            
                // We will start by moving the leading rows (up to jLow) down above
                // jHigh:
                memmove(dst, (void*)bitGrid->grid[channelIdx].b8, bitGrid->dimensions.nWordsPerRow * bitGrid->dimensions.nBytesPerWord * jLow);
            
                // Then we have to zero-out everything up to dst:
                memset((void*)bitGrid->grid[channelIdx].b8, 0, dst - (void*)bitGrid->grid[channelIdx].b8);
            }
        }
    }
}

//

static inline void
__TBitGridExtract4x4AtPosition_8b(
    TBitGrid        *bitGrid,
    unsigned int    channelIdx,
    int             j0,
    int             j4,
    int             i0,
    int             i4,
    int             baseW,
    int             inRowb,
    int             inRowBitCount,
    uint16_t        *extracted4x4Value,
    uint16_t        *extracted4x4Mask
)
{
    uint16_t        out4x4 = 0xFFFF;
    unsigned int    nRow = j4 - j0, inRowbHi = inRowb + inRowBitCount;
    uint16_t        isExtractedMask;
    uint16_t        extracted4x4;
    
    // Extract inRowBitCount bits at W,b
    if ( inRowbHi <= 8 ) {
        //  All in a single word:
        uint8_t     selectMask = (((uint8_t)1 << (inRowb + inRowBitCount)) - 1) & ~((1 << inRowb) - 1);
        int         shift;
        
        extracted4x4 = (uint16_t)((*(bitGrid->grid[channelIdx].b8 + baseW) & selectMask) >> inRowb);
        isExtractedMask = 0x00F;
        if ( --nRow > 0 ) {
            shift = 4 - inRowb;
            if ( shift <= 0 )
                extracted4x4 |= (uint16_t)(*(bitGrid->grid[channelIdx].b8 + baseW + bitGrid->dimensions.nWordsPerRow) & selectMask) >> -shift;
            else if ( shift > 0 )
                extracted4x4 |= (uint16_t)(*(bitGrid->grid[channelIdx].b8 + baseW + bitGrid->dimensions.nWordsPerRow) & selectMask) << shift;
            isExtractedMask = 0x00FF;
            if ( --nRow > 0 ) {
                shift += 4;
                if ( shift <= 0 )
                    extracted4x4 |= (uint16_t)(*(bitGrid->grid[channelIdx].b8 + baseW + 2 * bitGrid->dimensions.nWordsPerRow) & selectMask) >> -shift;
                else if ( shift > 0 )
                    extracted4x4 |= (uint16_t)(*(bitGrid->grid[channelIdx].b8 + baseW + 2 * bitGrid->dimensions.nWordsPerRow) & selectMask) << shift;
                isExtractedMask = 0x0FFF;
                if ( --nRow > 0 ) {
                    shift += 4;
                    if ( shift <= 0 )
                        extracted4x4 |= (uint16_t)(*(bitGrid->grid[channelIdx].b8 + baseW + 3 * bitGrid->dimensions.nWordsPerRow) & selectMask) >> -shift;
                    else if ( shift > 0 )
                        extracted4x4 |= (uint16_t)(*(bitGrid->grid[channelIdx].b8 + baseW + 3 * bitGrid->dimensions.nWordsPerRow) & selectMask) << shift;
                    isExtractedMask = 0xFFFF;
                }
            }
        }
    } else {
        // Split across two words:
        uint8_t     selectMask0 = ~((uint8_t)0xFF >> (8 - inRowb));
        uint8_t     selectMask1 = (((uint8_t)1 << (inRowbHi - 8)) - 1);
        int         shift0, shift1 = 8 - inRowb;
        
        extracted4x4 = ((uint16_t)(*(bitGrid->grid[channelIdx].b8 + baseW) & selectMask0) >> inRowb) |
                       ((uint16_t)(*(bitGrid->grid[channelIdx].b8 + baseW + 1) & selectMask1) << shift1);
        isExtractedMask = 0x000F;
        if ( --nRow > 0 ) {
            shift0 = 4 - inRowb;
            shift1 += 4;
            if ( shift0 <= 0 )
                extracted4x4 |= ((uint16_t)(*(bitGrid->grid[channelIdx].b8 + baseW + bitGrid->dimensions.nWordsPerRow) & selectMask0) >> -shift0) |
                          ((uint16_t)(*(bitGrid->grid[channelIdx].b8 + baseW + bitGrid->dimensions.nWordsPerRow + 1) & selectMask1) << shift1);
            else if ( shift0 > 0 )
                extracted4x4 |= ((uint16_t)(*(bitGrid->grid[channelIdx].b8 + baseW + bitGrid->dimensions.nWordsPerRow) & selectMask0) << shift0) |
                          ((uint16_t)(*(bitGrid->grid[channelIdx].b8 + baseW + bitGrid->dimensions.nWordsPerRow + 1) & selectMask1) << shift1);
            isExtractedMask = 0x00FF;
            if ( --nRow > 0 ) {
                shift0 += 4;
                shift1 += 4;
                if ( shift0 <= 0 )
                    extracted4x4 |= ((uint16_t)(*(bitGrid->grid[channelIdx].b8 + baseW + 2 * bitGrid->dimensions.nWordsPerRow) & selectMask0) >> -shift0) |
                              ((uint16_t)(*(bitGrid->grid[channelIdx].b8 + baseW + 2 * bitGrid->dimensions.nWordsPerRow + 1) & selectMask1) << shift1);
                else if ( shift0 > 0 )
                    extracted4x4 |= ((uint16_t)(*(bitGrid->grid[channelIdx].b8 + baseW + 2 * bitGrid->dimensions.nWordsPerRow) & selectMask0) << shift0) |
                              ((uint16_t)(*(bitGrid->grid[channelIdx].b8 + baseW + 2 * bitGrid->dimensions.nWordsPerRow + 1) & selectMask1) << shift1);
                isExtractedMask = 0x0FFF;
                if ( --nRow > 0 ) {
                    shift0 += 4;
                    shift1 += 4;
                    if ( shift0 <= 0 )
                        extracted4x4 |= ((uint16_t)(*(bitGrid->grid[channelIdx].b8 + baseW + 3 * bitGrid->dimensions.nWordsPerRow) & selectMask0) >> -shift0) |
                                  ((uint16_t)(*(bitGrid->grid[channelIdx].b8 + baseW + 3* bitGrid->dimensions.nWordsPerRow + 1) & selectMask1) << shift1);
                    else if ( shift0 > 0 )
                        extracted4x4 |= ((uint16_t)(*(bitGrid->grid[channelIdx].b8 + baseW + 3 * bitGrid->dimensions.nWordsPerRow) & selectMask0) << shift0) |
                                  ((uint16_t)(*(bitGrid->grid[channelIdx].b8 + baseW + 3* bitGrid->dimensions.nWordsPerRow + 1) & selectMask1) << shift1);
                    isExtractedMask = 0xFFFF;
                }
            }
        }
    }
    *extracted4x4Value = extracted4x4;
    *extracted4x4Mask = isExtractedMask;
}

static inline void
__TBitGridExtract4x4AtPosition_16b(
    TBitGrid        *bitGrid,
    unsigned int    channelIdx,
    int             j0,
    int             j4,
    int             i0,
    int             i4,
    int             baseW,
    int             inRowb,
    int             inRowBitCount,
    uint16_t        *extracted4x4Value,
    uint16_t        *extracted4x4Mask
)
{
    uint16_t        out4x4 = 0xFFFF;
    unsigned int    nRow = j4 - j0, inRowbHi = inRowb + inRowBitCount;
    uint16_t        isExtractedMask;
    uint16_t        extracted4x4;
    
    // Extract inRowBitCount bits at W,b
    if ( inRowbHi <= 16 ) {
        //  All in a single word:
        uint16_t    selectMask = (((uint16_t)1 << (inRowb + inRowBitCount)) - 1) & ~((1 << inRowb) - 1);
        int         shift;
        
        extracted4x4 = ((*(bitGrid->grid[channelIdx].b16 + baseW) & selectMask) >> inRowb);
        isExtractedMask = 0x00F;
        if ( --nRow > 0 ) {
            shift = 4 - inRowb;
            if ( shift <= 0 )
                extracted4x4 |= (*(bitGrid->grid[channelIdx].b16 + baseW + bitGrid->dimensions.nWordsPerRow) & selectMask) >> -shift;
            else if ( shift > 0 )
                extracted4x4 |= (*(bitGrid->grid[channelIdx].b16 + baseW + bitGrid->dimensions.nWordsPerRow) & selectMask) << shift;
            isExtractedMask = 0x00FF;
            if ( --nRow > 0 ) {
                shift += 4;
                if ( shift <= 0 )
                    extracted4x4 |= (*(bitGrid->grid[channelIdx].b16 + baseW + 2 * bitGrid->dimensions.nWordsPerRow) & selectMask) >> -shift;
                else if ( shift > 0 )
                    extracted4x4 |= (*(bitGrid->grid[channelIdx].b16 + baseW + 2 * bitGrid->dimensions.nWordsPerRow) & selectMask) << shift;
                isExtractedMask = 0x0FFF;
                if ( --nRow > 0 ) {
                    shift += 4;
                    if ( shift <= 0 )
                        extracted4x4 |= (*(bitGrid->grid[channelIdx].b16 + baseW + 3 * bitGrid->dimensions.nWordsPerRow) & selectMask) >> -shift;
                    else if ( shift > 0 )
                        extracted4x4 |= (*(bitGrid->grid[channelIdx].b16 + baseW + 3 * bitGrid->dimensions.nWordsPerRow) & selectMask) << shift;
                    isExtractedMask = 0xFFFF;
                }
            }
        }
    } else {
        // Split across two words:
        uint16_t    selectMask0 = ~((uint16_t)0xFFFF >> (16 - inRowb));
        uint16_t    selectMask1 = (((uint16_t)1 << (inRowbHi - 16)) - 1);
        int         shift0, shift1 = 16 - inRowb;
        
        extracted4x4 = ((*(bitGrid->grid[channelIdx].b16 + baseW) & selectMask0) >> inRowb) |
                       ((*(bitGrid->grid[channelIdx].b16 + baseW + 1) & selectMask1) << shift1);
        isExtractedMask = 0x000F;
        if ( --nRow > 0 ) {
            shift0 = 4 - inRowb;
            shift1 += 4;
            if ( shift0 <= 0 )
                extracted4x4 |= ((*(bitGrid->grid[channelIdx].b16 + baseW + bitGrid->dimensions.nWordsPerRow) & selectMask0) >> -shift0) |
                          ((*(bitGrid->grid[channelIdx].b16 + baseW + bitGrid->dimensions.nWordsPerRow + 1) & selectMask1) << shift1);
            else if ( shift0 > 0 )
                extracted4x4 |= ((*(bitGrid->grid[channelIdx].b16 + baseW + bitGrid->dimensions.nWordsPerRow) & selectMask0) << shift0) |
                          ((*(bitGrid->grid[channelIdx].b16 + baseW + bitGrid->dimensions.nWordsPerRow + 1) & selectMask1) << shift1);
            isExtractedMask = 0x00FF;
            if ( --nRow > 0 ) {
                shift0 += 4;
                shift1 += 4;
                if ( shift0 <= 0 )
                    extracted4x4 |= ((*(bitGrid->grid[channelIdx].b16 + baseW + 2 * bitGrid->dimensions.nWordsPerRow) & selectMask0) >> -shift0) |
                              ((*(bitGrid->grid[channelIdx].b16 + baseW + 2 * bitGrid->dimensions.nWordsPerRow + 1) & selectMask1) << shift1);
                else if ( shift0 > 0 )
                    extracted4x4 |= ((*(bitGrid->grid[channelIdx].b16 + baseW + 2 * bitGrid->dimensions.nWordsPerRow) & selectMask0) << shift0) |
                              ((*(bitGrid->grid[channelIdx].b16 + baseW + 2 * bitGrid->dimensions.nWordsPerRow + 1) & selectMask1) << shift1);
                isExtractedMask = 0x0FFF;
                if ( --nRow > 0 ) {
                    shift0 += 4;
                    shift1 += 4;
                    if ( shift0 <= 0 )
                        extracted4x4 |= ((*(bitGrid->grid[channelIdx].b16 + baseW + 3 * bitGrid->dimensions.nWordsPerRow) & selectMask0) >> -shift0) |
                                  ((*(bitGrid->grid[channelIdx].b16 + baseW + 3* bitGrid->dimensions.nWordsPerRow + 1) & selectMask1) << shift1);
                    else if ( shift0 > 0 )
                        extracted4x4 |= ((*(bitGrid->grid[channelIdx].b16 + baseW + 3 * bitGrid->dimensions.nWordsPerRow) & selectMask0) << shift0) |
                                  ((*(bitGrid->grid[channelIdx].b16 + baseW + 3* bitGrid->dimensions.nWordsPerRow + 1) & selectMask1) << shift1);
                    isExtractedMask = 0xFFFF;
                }
            }
        }
    }
    *extracted4x4Value = extracted4x4;
    *extracted4x4Mask = isExtractedMask;
}

static inline void
__TBitGridExtract4x4AtPosition_32b(
    TBitGrid        *bitGrid,
    unsigned int    channelIdx,
    int             j0,
    int             j4,
    int             i0,
    int             i4,
    int             baseW,
    int             inRowb,
    int             inRowBitCount,
    uint16_t        *extracted4x4Value,
    uint16_t        *extracted4x4Mask
)
{
    uint16_t        out4x4 = 0xFFFF;
    unsigned int    nRow = j4 - j0, inRowbHi = inRowb + inRowBitCount;
    uint16_t        isExtractedMask;
    uint16_t        extracted4x4;
    
    // Extract inRowBitCount bits at W,b
    if ( inRowbHi <= 32 ) {
        //  All in a single word:
        uint32_t    selectMask = (((uint32_t)1 << (inRowb + inRowBitCount)) - 1) & ~((1 << inRowb) - 1);
        int         shift;
        
        extracted4x4 = ((*(bitGrid->grid[channelIdx].b32 + baseW) & selectMask) >> inRowb);
        isExtractedMask = 0x00F;
        if ( --nRow > 0 ) {
            shift = 4 - inRowb;
            if ( shift <= 0 )
                extracted4x4 |= (*(bitGrid->grid[channelIdx].b32 + baseW + bitGrid->dimensions.nWordsPerRow) & selectMask) >> -shift;
            else if ( shift > 0 )
                extracted4x4 |= (*(bitGrid->grid[channelIdx].b32 + baseW + bitGrid->dimensions.nWordsPerRow) & selectMask) << shift;
            isExtractedMask = 0x00FF;
            if ( --nRow > 0 ) {
                shift += 4;
                if ( shift <= 0 )
                    extracted4x4 |= (*(bitGrid->grid[channelIdx].b32 + baseW + 2 * bitGrid->dimensions.nWordsPerRow) & selectMask) >> -shift;
                else if ( shift > 0 )
                    extracted4x4 |= (*(bitGrid->grid[channelIdx].b32 + baseW + 2 * bitGrid->dimensions.nWordsPerRow) & selectMask) << shift;
                isExtractedMask = 0x0FFF;
                if ( --nRow > 0 ) {
                    shift += 4;
                    if ( shift <= 0 )
                        extracted4x4 |= (*(bitGrid->grid[channelIdx].b32 + baseW + 3 * bitGrid->dimensions.nWordsPerRow) & selectMask) >> -shift;
                    else if ( shift > 0 )
                        extracted4x4 |= (*(bitGrid->grid[channelIdx].b32 + baseW + 3 * bitGrid->dimensions.nWordsPerRow) & selectMask) << shift;
                    isExtractedMask = 0xFFFF;
                }
            }
        }
    } else {
        // Split across two words:
        uint32_t    selectMask0 = ~((uint32_t)0xFFFFFFFF >> (32 - inRowb));
        uint32_t    selectMask1 = (((uint32_t)1 << (inRowbHi - 32)) - 1);
        int         shift0, shift1 = 32 - inRowb;
        
        extracted4x4 = ((*(bitGrid->grid[channelIdx].b32 + baseW) & selectMask0) >> inRowb) |
                       ((*(bitGrid->grid[channelIdx].b32 + baseW + 1) & selectMask1) << shift1);
        isExtractedMask = 0x000F;
        if ( --nRow > 0 ) {
            shift0 = 4 - inRowb;
            shift1 += 4;
            if ( shift0 <= 0 )
                extracted4x4 |= ((*(bitGrid->grid[channelIdx].b32 + baseW + bitGrid->dimensions.nWordsPerRow) & selectMask0) >> -shift0) |
                          ((*(bitGrid->grid[channelIdx].b32 + baseW + bitGrid->dimensions.nWordsPerRow + 1) & selectMask1) << shift1);
            else if ( shift0 > 0 )
                extracted4x4 |= ((*(bitGrid->grid[channelIdx].b32 + baseW + bitGrid->dimensions.nWordsPerRow) & selectMask0) << shift0) |
                          ((*(bitGrid->grid[channelIdx].b32 + baseW + bitGrid->dimensions.nWordsPerRow + 1) & selectMask1) << shift1);
            isExtractedMask = 0x00FF;
            if ( --nRow > 0 ) {
                shift0 += 4;
                shift1 += 4;
                if ( shift0 <= 0 )
                    extracted4x4 |= ((*(bitGrid->grid[channelIdx].b32 + baseW + 2 * bitGrid->dimensions.nWordsPerRow) & selectMask0) >> -shift0) |
                              ((*(bitGrid->grid[channelIdx].b32 + baseW + 2 * bitGrid->dimensions.nWordsPerRow + 1) & selectMask1) << shift1);
                else if ( shift0 > 0 )
                    extracted4x4 |= ((*(bitGrid->grid[channelIdx].b32 + baseW + 2 * bitGrid->dimensions.nWordsPerRow) & selectMask0) << shift0) |
                              ((*(bitGrid->grid[channelIdx].b32 + baseW + 2 * bitGrid->dimensions.nWordsPerRow + 1) & selectMask1) << shift1);
                isExtractedMask = 0x0FFF;
                if ( --nRow > 0 ) {
                    shift0 += 4;
                    shift1 += 4;
                    if ( shift0 <= 0 )
                        extracted4x4 |= ((*(bitGrid->grid[channelIdx].b32 + baseW + 3 * bitGrid->dimensions.nWordsPerRow) & selectMask0) >> -shift0) |
                                  ((*(bitGrid->grid[channelIdx].b32 + baseW + 3* bitGrid->dimensions.nWordsPerRow + 1) & selectMask1) << shift1);
                    else if ( shift0 > 0 )
                        extracted4x4 |= ((*(bitGrid->grid[channelIdx].b32 + baseW + 3 * bitGrid->dimensions.nWordsPerRow) & selectMask0) << shift0) |
                                  ((*(bitGrid->grid[channelIdx].b32 + baseW + 3* bitGrid->dimensions.nWordsPerRow + 1) & selectMask1) << shift1);
                    isExtractedMask = 0xFFFF;
                }
            }
        }
    }
    *extracted4x4Value = extracted4x4;
    *extracted4x4Mask = isExtractedMask;
}

static inline void
__TBitGridExtract4x4AtPosition_64b(
    TBitGrid        *bitGrid,
    unsigned int    channelIdx,
    int             j0,
    int             j4,
    int             i0,
    int             i4,
    int             baseW,
    int             inRowb,
    int             inRowBitCount,
    uint16_t        *extracted4x4Value,
    uint16_t        *extracted4x4Mask
)
{
    uint16_t        out4x4 = 0xFFFF;
    unsigned int    nRow = j4 - j0, inRowbHi = inRowb + inRowBitCount;
    uint16_t        isExtractedMask;
    uint16_t        extracted4x4;
    
    // Extract inRowBitCount bits at W,b
    if ( inRowbHi <= 64 ) {
        //  All in a single word:
        uint64_t    selectMask = (((uint64_t)1 << (inRowb + inRowBitCount)) - 1) & ~((1 << inRowb) - 1);
        int         shift;
        
        extracted4x4 = ((*(bitGrid->grid[channelIdx].b64 + baseW) & selectMask) >> inRowb);
        isExtractedMask = 0x00F;
        if ( --nRow > 0 ) {
            shift = 4 - inRowb;
            if ( shift <= 0 )
                extracted4x4 |= (*(bitGrid->grid[channelIdx].b64 + baseW + bitGrid->dimensions.nWordsPerRow) & selectMask) >> -shift;
            else if ( shift > 0 )
                extracted4x4 |= (*(bitGrid->grid[channelIdx].b64 + baseW + bitGrid->dimensions.nWordsPerRow) & selectMask) << shift;
            isExtractedMask = 0x00FF;
            if ( --nRow > 0 ) {
                shift += 4;
                if ( shift <= 0 )
                    extracted4x4 |= (*(bitGrid->grid[channelIdx].b64 + baseW + 2 * bitGrid->dimensions.nWordsPerRow) & selectMask) >> -shift;
                else if ( shift > 0 )
                    extracted4x4 |= (*(bitGrid->grid[channelIdx].b64 + baseW + 2 * bitGrid->dimensions.nWordsPerRow) & selectMask) << shift;
                isExtractedMask = 0x0FFF;
                if ( --nRow > 0 ) {
                    shift += 4;
                    if ( shift <= 0 )
                        extracted4x4 |= (*(bitGrid->grid[channelIdx].b64 + baseW + 3 * bitGrid->dimensions.nWordsPerRow) & selectMask) >> -shift;
                    else if ( shift > 0 )
                        extracted4x4 |= (*(bitGrid->grid[channelIdx].b64 + baseW + 3 * bitGrid->dimensions.nWordsPerRow) & selectMask) << shift;
                    isExtractedMask = 0xFFFF;
                }
            }
        }
    } else {
        // Split across two words:
        uint64_t    selectMask0 = ~((uint64_t)0xFFFFFFFFFFFFFFFF >> (64 - inRowb));
        uint64_t    selectMask1 = (((uint64_t)1 << (inRowbHi - 64)) - 1);
        int         shift0, shift1 = 64 - inRowb;
        
        extracted4x4 = ((*(bitGrid->grid[channelIdx].b64 + baseW) & selectMask0) >> inRowb) |
                       ((*(bitGrid->grid[channelIdx].b64 + baseW + 1) & selectMask1) << shift1);
        isExtractedMask = 0x000F;
        if ( --nRow > 0 ) {
            shift0 = 4 - inRowb;
            shift1 += 4;
            if ( shift0 <= 0 )
                extracted4x4 |= ((*(bitGrid->grid[channelIdx].b64 + baseW + bitGrid->dimensions.nWordsPerRow) & selectMask0) >> -shift0) |
                          ((*(bitGrid->grid[channelIdx].b64 + baseW + bitGrid->dimensions.nWordsPerRow + 1) & selectMask1) << shift1);
            else if ( shift0 > 0 )
                extracted4x4 |= ((*(bitGrid->grid[channelIdx].b64 + baseW + bitGrid->dimensions.nWordsPerRow) & selectMask0) << shift0) |
                          ((*(bitGrid->grid[channelIdx].b64 + baseW + bitGrid->dimensions.nWordsPerRow + 1) & selectMask1) << shift1);
            isExtractedMask = 0x00FF;
            if ( --nRow > 0 ) {
                shift0 += 4;
                shift1 += 4;
                if ( shift0 <= 0 )
                    extracted4x4 |= ((*(bitGrid->grid[channelIdx].b64 + baseW + 2 * bitGrid->dimensions.nWordsPerRow) & selectMask0) >> -shift0) |
                              ((*(bitGrid->grid[channelIdx].b64 + baseW + 2 * bitGrid->dimensions.nWordsPerRow + 1) & selectMask1) << shift1);
                else if ( shift0 > 0 )
                    extracted4x4 |= ((*(bitGrid->grid[channelIdx].b64 + baseW + 2 * bitGrid->dimensions.nWordsPerRow) & selectMask0) << shift0) |
                              ((*(bitGrid->grid[channelIdx].b64 + baseW + 2 * bitGrid->dimensions.nWordsPerRow + 1) & selectMask1) << shift1);
                isExtractedMask = 0x0FFF;
                if ( --nRow > 0 ) {
                    shift0 += 4;
                    shift1 += 4;
                    if ( shift0 <= 0 )
                        extracted4x4 |= ((*(bitGrid->grid[channelIdx].b64 + baseW + 3 * bitGrid->dimensions.nWordsPerRow) & selectMask0) >> -shift0) |
                                  ((*(bitGrid->grid[channelIdx].b64 + baseW + 3* bitGrid->dimensions.nWordsPerRow + 1) & selectMask1) << shift1);
                    else if ( shift0 > 0 )
                        extracted4x4 |= ((*(bitGrid->grid[channelIdx].b64 + baseW + 3 * bitGrid->dimensions.nWordsPerRow) & selectMask0) << shift0) |
                                  ((*(bitGrid->grid[channelIdx].b64 + baseW + 3* bitGrid->dimensions.nWordsPerRow + 1) & selectMask1) << shift1);
                    isExtractedMask = 0xFFFF;
                }
            }
        }
    }
    *extracted4x4Value = extracted4x4;
    *extracted4x4Mask = isExtractedMask;
}

uint16_t
TBitGridExtract4x4AtPosition(
    TBitGrid        *bitGrid,
    unsigned int    channelIdx,
    TGridPos        P
)
{
    uint16_t    out4x4, out4x4Mask;
    int         baseW, inRowb, inRowBitCount;
    int         i0, i4, j0, j4;
    
    if ( channelIdx-- > bitGrid->dimensions.nChannels ) return 0xFFFF;
    
    // Off-grid:
    if ( P.i < -3 || P.j < -3 ) return 0xFFFF;
    if ( ((P.i > 0) && (P.i >= bitGrid->dimensions.w)) || ((P.j > 0) && (P.j >= bitGrid->dimensions.h)) ) return 0xFFFF;
    
    // Row and column range -- clamp at the right and bottom:
    j0 = P.j; j4 = j0 + 4; if ( j4 > bitGrid->dimensions.h ) j4 = bitGrid->dimensions.h;
    i0 = P.i; i4 = i0 + 4; if ( i4 > bitGrid->dimensions.w ) i4 = bitGrid->dimensions.w;
    inRowBitCount = i4 - i0;
    
    // Calculate the word/bit offset at which we'll start extracting bits:
    baseW  = ((P.j < 0) ? (P.j + 4) : P.j) * bitGrid->dimensions.nWordsPerRow +
             ((P.i < 0) ? 0 : P.i) / bitGrid->dimensions.nBitsPerWord;
    inRowb = ((P.i < 0) ? 0 : P.i) % bitGrid->dimensions.nBitsPerWord;
    
    switch ( bitGrid->dimensions.nBitsPerWord ) {
        case 8:
            __TBitGridExtract4x4AtPosition_8b(bitGrid, channelIdx, j0, j4, i0, i4, baseW, inRowb, inRowBitCount, &out4x4, &out4x4Mask);
            break;
        case 16:
            __TBitGridExtract4x4AtPosition_16b(bitGrid, channelIdx, j0, j4, i0, i4, baseW, inRowb, inRowBitCount, &out4x4, &out4x4Mask);
            break;
        case 32:
            __TBitGridExtract4x4AtPosition_32b(bitGrid, channelIdx, j0, j4, i0, i4, baseW, inRowb, inRowBitCount, &out4x4, &out4x4Mask);
            break;
        case 64:
            __TBitGridExtract4x4AtPosition_64b(bitGrid, channelIdx, j0, j4, i0, i4, baseW, inRowb, inRowBitCount, &out4x4, &out4x4Mask);
            break;
    }
    
    // Left-right shifts:
    i4 = i0 + 4;
    if ( i0 < 0 ) {
        out4x4 = ((0xFFFF & ~out4x4Mask) | out4x4);
        while ( i0++ < 0 ) {
            // Take the left 3 columns and shift them right, then
            // set the 0th column to all occupied (since it's off
            // the grid):
            out4x4 = (out4x4 & 0x7777) << 1 | 0x1111;
        }
    } else if ( i4 > bitGrid->dimensions.w ) {
        // Shift the out4x4Mask to only keep the appropriate columns:
        i4 -= bitGrid->dimensions.w;
        while ( i4-- > 0 ) out4x4Mask = (out4x4Mask >> 1) & 0x7777;
        out4x4 = (out4x4 & out4x4Mask) | (0xFFFF & ~out4x4Mask);
    } else {
        out4x4 = ((0xFFFF & ~out4x4Mask) | out4x4);
    }
    
    return out4x4;
}

//

void
__TBitGridSet4x4AtPosition_8b(
    TBitGrid        *bitGrid,
    unsigned int    channelIdx,
    int             iLo,
    int             iHi,
    int             jLo,
    int             jHi,
    unsigned int    baseW,
    unsigned int    baseb,
    uint16_t        in4x4
)
{
    uint8_t         *grid = bitGrid->grid[channelIdx].b8 + baseW;
    unsigned int    nBitsInRow = iHi - iLo;
    uint16_t        in4x4Mask = ((1 << nBitsInRow) - 1);
    
    if ( baseb + nBitsInRow <= 8 ) {
        int         shift = baseb;
        
        // All in the same word:
        while ( jLo < jHi ) {
            if ( shift <= 0 )
                *grid |= ((in4x4 & in4x4Mask) >> -shift);
            else
                *grid |= ((in4x4 & in4x4Mask) << shift);
            grid += bitGrid->dimensions.nWordsPerRow;
            in4x4Mask <<= 4;
            shift -= 4;
            jLo++;
        }
    } else {
        int         shift0 = baseb, shift1 = baseb - 4;
        
        // Split between two consecutive words:
        while ( jLo < jHi ) {
            if ( shift0 <= 0)
                *grid |= (in4x4 & in4x4Mask) << shift0;
            else
                *grid |= (in4x4 & in4x4Mask) >> -shift0;
            if ( shift1 < 0 )
                *(grid + 1) |= (in4x4 & in4x4Mask) >> shift1;
            else
                *(grid + 1) |= (in4x4 & in4x4Mask) << -shift1;
            grid += bitGrid->dimensions.nWordsPerRow;
            in4x4Mask <<= 4;
            shift0 -= 4;
            shift1 += 4;
            jLo++;
        }
    }
}

void
__TBitGridSet4x4AtPosition_16b(
    TBitGrid        *bitGrid,
    unsigned int    channelIdx,
    int             iLo,
    int             iHi,
    int             jLo,
    int             jHi,
    unsigned int    baseW,
    unsigned int    baseb,
    uint16_t        in4x4
)
{
    uint16_t        *grid = bitGrid->grid[channelIdx].b16 + baseW;
    unsigned int    nBitsInRow = iHi - iLo;
    uint16_t        in4x4Mask = ((1 << nBitsInRow) - 1);
    
    if ( baseb + nBitsInRow <= 16 ) {
        int         shift = baseb;
        
        // All in the same word:
        while ( jLo < jHi ) {
            if ( shift <= 0 )
                *grid |= ((in4x4 & in4x4Mask) >> -shift);
            else
                *grid |= ((in4x4 & in4x4Mask) << shift);
            grid += bitGrid->dimensions.nWordsPerRow;
            in4x4Mask <<= 4;
            shift -= 4;
            jLo++;
        }
    } else {
        int         shift0 = baseb, shift1 = baseb - 12;
        
        // Split between two consecutive words:
        while ( jLo < jHi ) {
            if ( shift0 <= 0)
                *grid |= (in4x4 & in4x4Mask) << shift0;
            else
                *grid |= (in4x4 & in4x4Mask) >> -shift0;
            if ( shift1 < 0 )
                *(grid + 1) |= (in4x4 & in4x4Mask) >> shift1;
            else
                *(grid + 1) |= (in4x4 & in4x4Mask) << -shift1;
            grid += bitGrid->dimensions.nWordsPerRow;
            in4x4Mask <<= 4;
            shift0 -= 4;
            shift1 += 4;
            jLo++;
        }
    }
}

void
__TBitGridSet4x4AtPosition_32b(
    TBitGrid        *bitGrid,
    unsigned int    channelIdx,
    int             iLo,
    int             iHi,
    int             jLo,
    int             jHi,
    unsigned int    baseW,
    unsigned int    baseb,
    uint16_t        in4x4
)
{
    uint32_t        *grid = bitGrid->grid[channelIdx].b32 + baseW;
    unsigned int    nBitsInRow = iHi - iLo;
    uint16_t        in4x4Mask = ((1 << nBitsInRow) - 1);
    
    if ( baseb + nBitsInRow <= 32 ) {
        int         shift = baseb;
        
        // All in the same word:
        while ( jLo < jHi ) {
            if ( shift <= 0 )
                *grid |= ((in4x4 & in4x4Mask) >> -shift);
            else
                *grid |= ((in4x4 & in4x4Mask) << shift);
            grid += bitGrid->dimensions.nWordsPerRow;
            in4x4Mask <<= 4;
            shift -= 4;
            jLo++;
        }
    } else {
        int         shift0 = baseb, shift1 = baseb - 28;
        
        // Split between two consecutive words:
        while ( jLo < jHi ) {
            if ( shift0 <= 0)
                *grid |= (in4x4 & in4x4Mask) << shift0;
            else
                *grid |= (in4x4 & in4x4Mask) >> -shift0;
            if ( shift1 < 0 )
                *(grid + 1) |= (in4x4 & in4x4Mask) >> shift1;
            else
                *(grid + 1) |= (in4x4 & in4x4Mask) << -shift1;
            grid += bitGrid->dimensions.nWordsPerRow;
            in4x4Mask <<= 4;
            shift0 -= 4;
            shift1 += 4;
            jLo++;
        }
    }
}

void
__TBitGridSet4x4AtPosition_64b(
    TBitGrid        *bitGrid,
    unsigned int    channelIdx,
    int             iLo,
    int             iHi,
    int             jLo,
    int             jHi,
    unsigned int    baseW,
    unsigned int    baseb,
    uint16_t        in4x4
)
{
    uint64_t        *grid = bitGrid->grid[channelIdx].b64 + baseW;
    unsigned int    nBitsInRow = iHi - iLo;
    uint16_t        in4x4Mask = ((1 << nBitsInRow) - 1);
    
    if ( baseb + nBitsInRow <= 64 ) {
        int         shift = baseb;
        
        // All in the same word:
        while ( jLo < jHi ) {
            if ( shift <= 0 )
                *grid |= ((in4x4 & in4x4Mask) >> -shift);
            else
                *grid |= ((in4x4 & in4x4Mask) << shift);
            grid += bitGrid->dimensions.nWordsPerRow;
            in4x4Mask <<= 4;
            shift -= 4;
            jLo++;
        }
    } else {
        int         shift0 = baseb, shift1 = baseb - 60;
        
        // Split between two consecutive words:
        while ( jLo < jHi ) {
            if ( shift0 <= 0)
                *grid |= (in4x4 & in4x4Mask) << shift0;
            else
                *grid |= (in4x4 & in4x4Mask) >> -shift0;
            if ( shift1 < 0 )
                *(grid + 1) |= (in4x4 & in4x4Mask) >> shift1;
            else
                *(grid + 1) |= (in4x4 & in4x4Mask) << -shift1;
            grid += bitGrid->dimensions.nWordsPerRow;
            in4x4Mask <<= 4;
            shift0 -= 4;
            shift1 += 4;
            jLo++;
        }
    }
}

void
TBitGridSet4x4AtPosition(
    TBitGrid        *bitGrid,
    unsigned int    channelIdx,
    TGridPos        P,
    uint16_t        in4x4
)
{
    int             iLo = P.i, jLo = P.j;
    int             iHi = iLo + 4, jHi = jLo + 4;
    unsigned int    baseW, baseb;
    
    //  Off the top-left of the board:
    if ( iHi < 0 || jHi < 0 ) return;
    
    //  Off the right-bottom of the board:
    if ( iLo >= (int)bitGrid->dimensions.w || jLo >= (int)bitGrid->dimensions.h ) return;
    
    // Shift away any rows that are off the top of the board:
    while ( jLo < 0 ) {
        in4x4 >>= 4;
        jLo++;
    }
    
    // Shift away any rows that are off the left of the board:
    while ( iLo < 0 ) {
        in4x4 = (in4x4 & 0xEEEE) >> 1;
        iLo++;
    }
    
    // At this point (iLo,jLo) is the starting coordinate on the
    // grid.  Go ahead and calculate the word/bit offset:
    baseW = (jLo * bitGrid->dimensions.nWordsPerRow) + (iLo / bitGrid->dimensions.nBitsPerWord);
    baseb = (iLo % bitGrid->dimensions.nBitsPerWord);
    
    switch ( bitGrid->dimensions.nBitsPerWord ) {
        case 8:
            __TBitGridSet4x4AtPosition_8b(bitGrid, channelIdx, iLo, iHi, jLo, jHi, baseW, baseb, in4x4);
            break;
        case 16:
            __TBitGridSet4x4AtPosition_16b(bitGrid, channelIdx, iLo, iHi, jLo, jHi, baseW, baseb, in4x4);
            break;
        case 32:
            __TBitGridSet4x4AtPosition_32b(bitGrid, channelIdx, iLo, iHi, jLo, jHi, baseW, baseb, in4x4);
            break;
        case 64:
            __TBitGridSet4x4AtPosition_64b(bitGrid, channelIdx, iLo, iHi, jLo, jHi, baseW, baseb, in4x4);
            break;
    }
            
}

//

void
TBitGridChannelSummary(
    TBitGrid        *bitGrid,
    unsigned int    channelIdx
)
{
    TBitGridIterator    *iterator = TBitGridIteratorCreate(bitGrid, channelIdx);
    unsigned int        i, j = bitGrid->dimensions.h;
    TGridPos            P;
    
    printf("\n");
    while ( j-- ) {
        //
        // The Unicode big box uses 3 bytes for each character in UTF-8, so
        // we need to reserve 9 bytes per block per line:
        //
        static uint8_t      tBlockTop[10] = { 0xE2, 0x94, 0x8F, 0xE2, 0x94, 0x81, 0xE2, 0x94, 0x93, '\0' };
        static uint8_t      tBlockBot[10] = { 0xE2, 0x94, 0x97, 0xE2, 0x94, 0x81, 0xE2, 0x94, 0x9B, '\0' };
        char                line1[bitGrid->dimensions.w * 9 + 1], *line1Ptr = line1;
        char                line2[bitGrid->dimensions.w * 9 + 1], *line2Ptr = line2;
        int                 line1Len = sizeof(line1), line2Len = sizeof(line2);
        
        i = bitGrid->dimensions.w;
        while ( i-- ) {
            int     l;
            TCell   value;
            
            TBitGridIteratorNext(iterator, &P, &value);
            if ( value ) {
                l = snprintf(line1Ptr, line1Len, "%s", (const char*)tBlockTop); line1Len -= l; line1Ptr += l;
                l = snprintf(line2Ptr, line2Len, "%s", (const char*)tBlockBot); line2Len -= l; line2Ptr += l;
            } else {
                l = snprintf(line1Ptr, line1Len, "   "); line1Len -= l; line1Ptr += l;
                l = snprintf(line2Ptr, line2Len, "   "); line2Len -= l; line2Ptr += l;
            }
        }
        printf("%s\n%s\n", line1, line2);
    }
    TBitGridIteratorDestroy(iterator);
}

//
////
//

TBitGridIterator*
TBitGridIteratorCreate(
    TBitGrid    *bitGrid,
    TCell       channelMask
)
{
    size_t              gridBytes = bitGrid->dimensions.nChannels * sizeof(TBitGridChannelPtr);
    TBitGridIterator    *iterator = NULL;
    unsigned int        nChannelsEnabled = 0;
    TCell               channelMaskCopy;
    uint16_t            validChannelsMask = (1 << bitGrid->dimensions.nChannels) - 1;
    
    channelMask &= validChannelsMask;
    channelMaskCopy = channelMask;
    
    if ( channelMask == 0 ) return NULL;
    
    while ( channelMask ) {
        if ( channelMask & 0x1 ) nChannelsEnabled++;
        channelMask >>= 1;
    }
    if ( nChannelsEnabled ) {
        if ( nChannelsEnabled == 1 ) {
            TBitGridIterator_1C *ITERATOR = (TBitGridIterator_1C*)malloc(sizeof(TBitGridIterator_1C) + gridBytes);
            
            if ( ITERATOR ) {
                ITERATOR->base.grid = (void*)ITERATOR + sizeof(TBitGridIterator_1C);
                channelMask = 0;
                while ( (1 << channelMask) != channelMaskCopy ) channelMask++;
                ITERATOR->channelIdx = channelMask;
                iterator = (TBitGridIterator*)ITERATOR;
                switch ( bitGrid->dimensions.nBitsPerWord ) {
                    case 8:
                        iterator->callbacks.nextFn = __TBitGridIteratorNext_8b_1C;
                        iterator->callbacks.nextFullRowFn = __TBitGridIteratorNextFullRow_8b_1C;
                        break;
                    case 16:
                        iterator->callbacks.nextFn = __TBitGridIteratorNext_16b_1C;
                        iterator->callbacks.nextFullRowFn = __TBitGridIteratorNextFullRow_16b_1C;
                        break;
                    case 32:
                        iterator->callbacks.nextFn = __TBitGridIteratorNext_32b_1C;
                        iterator->callbacks.nextFullRowFn = __TBitGridIteratorNextFullRow_32b_1C;
                        break;
                    case 64:
                        iterator->callbacks.nextFn = __TBitGridIteratorNext_64b_1C;
                        iterator->callbacks.nextFullRowFn = __TBitGridIteratorNextFullRow_64b_1C;
                        break;
                }
            }
        } else {
            TBitGridIterator_NC *ITERATOR = (TBitGridIterator_NC*)malloc(sizeof(TBitGridIterator_NC) + gridBytes);
            
            if ( ITERATOR ) {
                ITERATOR->base.grid = (void*)ITERATOR + sizeof(TBitGridIterator_NC);
                ITERATOR->channelMask = channelMaskCopy;
                iterator = (TBitGridIterator*)ITERATOR;
                switch ( bitGrid->dimensions.nBitsPerWord ) {
                    case 8:
                        iterator->callbacks.nextFn = __TBitGridIteratorNext_8b_NC;
                        iterator->callbacks.nextFullRowFn = __TBitGridIteratorNextFullRow_8b_NC;
                        break;
                    case 16:
                        iterator->callbacks.nextFn = __TBitGridIteratorNext_16b_NC;
                        iterator->callbacks.nextFullRowFn = __TBitGridIteratorNextFullRow_16b_NC;
                        break;
                    case 32:
                        iterator->callbacks.nextFn = __TBitGridIteratorNext_32b_NC;
                        iterator->callbacks.nextFullRowFn = __TBitGridIteratorNextFullRow_32b_NC;
                        break;
                    case 64:
                        iterator->callbacks.nextFn = __TBitGridIteratorNext_64b_NC;
                        iterator->callbacks.nextFullRowFn = __TBitGridIteratorNextFullRow_64b_NC;
                        break;
                }
            }
        }
        if ( iterator ) {
            iterator->dimensions = bitGrid->dimensions;
            iterator->i = iterator->j = 0;
            iterator->nFullWords = bitGrid->dimensions.w / bitGrid->dimensions.nBitsPerWord;
            iterator->nPartialBits = bitGrid->dimensions.w % bitGrid->dimensions.nBitsPerWord;
            iterator->isStarted = false;
            memcpy(iterator->grid, bitGrid->grid, gridBytes);
        }
    }
    return iterator;
}

//

void
TBitGridIteratorDestroy(
    TBitGridIterator*   iterator
)
{
    free((void*)iterator);
}

//
////
//

#ifdef TBITGRID_DEMO

int
main()
{
    TBitGridIterator*   iter;
    TBitGrid            *gameBoard;
    TGridPos            P;
    TCell               value;
    unsigned int        i, j;
    
    // Test out a single-bit basic game board:
    gameBoard = TBitGridCreate(TBitGridWordSizeForce32Bit, 4, 10, 20);
    
    j = 0;
    while ( j < 20 ) {
        i = 0;
        while ( i < 10 ) {
            if ( ! (i % 2) || ! (j % 5) ) {
                TCell       v = 0x8 | (((i % 2) == 0) << 1) | ((j % 5) == 0) | ((random() % 2) << 2);
                TBitGridSetValueAtIndex(gameBoard, TBitGridMakeGridIndexWithPos(gameBoard, i, j), v);
            }
            i++;
        }
        j++;
    }
    printf("\n-------\n");
    j = 0;
    while ( j < 20 ) {
        i = 0;
        while ( i < 10 ) {
            printf("(%2u,%2u) = %02hhX\n", i, j, TBitGridGetValueAtIndex(gameBoard, TBitGridMakeGridIndexWithPos(gameBoard, i, j)));
            i++;
        }
        j++;
    }
    printf("\n-------\n");
    iter = TBitGridIteratorCreate(gameBoard, 0xF);
    while ( TBitGridIteratorNext(iter, &P, &value) ) {
        if ( value ) printf("(%2u,%2u) = %02hhX == %02hhX\n", P.i, P.j, value, ( ! (i % 2) || ! (j % 5) ) );
    }
    TBitGridIteratorDestroy(iter);
    
    printf("\n");
    
    iter = TBitGridIteratorCreate(gameBoard, 0x9);
    while ( TBitGridIteratorNextFullRow(iter, &j) ) {
        printf("ROW %u\n", j);
    }
    TBitGridIteratorDestroy(iter);
    
    free((void*)gameBoard);
    return(0);
}


#endif
