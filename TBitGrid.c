
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
    uint8_t                 channelMask;
} TBitGridIterator_NC;

//

bool
__TBitGridIteratorNext_8b_1C(
    TBitGridIterator    *iterator,
    TGridPos            *outP,
    uint8_t             *value
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
    uint8_t             *value
)
{
#define ITERATOR    ((TBitGridIterator_NC*)iterator)

    if ( iterator->j < iterator->dimensions.h ) {
        uint8_t     channelMask, channelIdx, localValue;
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
    uint8_t             *value
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
    uint8_t             *value
)
{
#define ITERATOR    ((TBitGridIterator_NC*)iterator)

    if ( iterator->j < iterator->dimensions.h ) {
        uint8_t     channelMask, channelIdx, localValue;
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
    uint8_t             *value
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
    uint8_t             *value
)
{
#define ITERATOR    ((TBitGridIterator_NC*)iterator)

    if ( iterator->j < iterator->dimensions.h ) {
        uint8_t     channelMask, channelIdx, localValue;
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
    uint8_t             *value
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
    uint8_t             *value
)
{
#define ITERATOR    ((TBitGridIterator_NC*)iterator)

    if ( iterator->j < iterator->dimensions.h ) {
        uint8_t     channelMask, channelIdx, localValue;
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

uint8_t
__TBitGridGetCellValueAtIndex_8b_1C(
    TBitGrid        *bitGrid,
    TGridIndex      I
)
{
    return ((bitGrid->grid[0].b8[I.W]) & (1 << I.b)) != 0;
}
uint8_t
__TBitGridGetCellValueAtIndex_8b_2C(
    TBitGrid        *bitGrid,
    TGridIndex      I
)
{
    return ((((bitGrid->grid[1].b8[I.W]) & (1 << I.b)) != 0) << 1) |
            (((bitGrid->grid[0].b8[I.W]) & (1 << I.b)) != 0);
}
uint8_t
__TBitGridGetCellValueAtIndex_8b_NC(
    TBitGrid        *bitGrid,
    TGridIndex      I
)
{
    uint8_t         outValue = 0;
    unsigned int    c = bitGrid->dimensions.nChannels;
    
    while ( c-- )
        outValue = (outValue << 1) | (((bitGrid->grid[c].b8[I.W]) & (1 << I.b)) != 0);
    return outValue;
}

void
__TBitGridSetCellValueAtIndex_8b_1C(
    TBitGrid        *bitGrid,
    TGridIndex      I,
    uint8_t         value
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
    uint8_t         value
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
    uint8_t         value
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

uint8_t
__TBitGridGetCellValueAtIndex_16b_1C(
    TBitGrid        *bitGrid,
    TGridIndex      I
)
{
    return ((bitGrid->grid[0].b16[I.W]) & (1 << I.b)) != 0;
}
uint8_t
__TBitGridGetCellValueAtIndex_16b_2C(
    TBitGrid        *bitGrid,
    TGridIndex      I
)
{
    return ((((bitGrid->grid[1].b16[I.W]) & (1 << I.b)) != 0) << 1) |
            (((bitGrid->grid[0].b16[I.W]) & (1 << I.b)) != 0);
}
uint8_t
__TBitGridGetCellValueAtIndex_16b_NC(
    TBitGrid        *bitGrid,
    TGridIndex      I
)
{
    uint8_t         outValue = 0;
    unsigned int    c = bitGrid->dimensions.nChannels;
    
    while ( c-- )
        outValue = (outValue << 1) | (((bitGrid->grid[c].b16[I.W]) & (1 << I.b)) != 0);
    return outValue;
}

void
__TBitGridSetCellValueAtIndex_16b_1C(
    TBitGrid        *bitGrid,
    TGridIndex      I,
    uint8_t         value
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
    uint8_t         value
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
    uint8_t         value
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

uint8_t
__TBitGridGetCellValueAtIndex_32b_1C(
    TBitGrid        *bitGrid,
    TGridIndex      I
)
{
    return ((bitGrid->grid[0].b32[I.W]) & (1 << I.b)) != 0;
}
uint8_t
__TBitGridGetCellValueAtIndex_32b_2C(
    TBitGrid        *bitGrid,
    TGridIndex      I
)
{
    return ((((bitGrid->grid[1].b32[I.W]) & (1 << I.b)) != 0) << 1) |
            (((bitGrid->grid[0].b32[I.W]) & (1 << I.b)) != 0);
}
uint8_t
__TBitGridGetCellValueAtIndex_32b_NC(
    TBitGrid        *bitGrid,
    TGridIndex      I
)
{
    uint8_t         outValue = 0;
    unsigned int    c = bitGrid->dimensions.nChannels;
    
    while ( c-- )
        outValue = (outValue << 1) | (((bitGrid->grid[c].b32[I.W]) & (1 << I.b)) != 0);
    return outValue;
}

void
__TBitGridSetCellValueAtIndex_32b_1C(
    TBitGrid        *bitGrid,
    TGridIndex      I,
    uint8_t         value
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
    uint8_t         value
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
    uint8_t         value
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

uint8_t
__TBitGridGetCellValueAtIndex_64b_1C(
    TBitGrid        *bitGrid,
    TGridIndex      I
)
{
    return ((bitGrid->grid[0].b64[I.W]) & (1 << I.b)) != 0;
}
uint8_t
__TBitGridGetCellValueAtIndex_64b_2C(
    TBitGrid        *bitGrid,
    TGridIndex      I
)
{
    return ((((bitGrid->grid[1].b64[I.W]) & (1 << I.b)) != 0) << 1) |
            (((bitGrid->grid[0].b64[I.W]) & (1 << I.b)) != 0);
}
uint8_t
__TBitGridGetCellValueAtIndex_64b_NC(
    TBitGrid        *bitGrid,
    TGridIndex      I
)
{
    uint8_t         outValue = 0;
    unsigned int    c = bitGrid->dimensions.nChannels;
    
    while ( c-- )
        outValue = (outValue << 1) | (((bitGrid->grid[c].b64[I.W]) & (1 << I.b)) != 0);
    return outValue;
}

void
__TBitGridSetCellValueAtIndex_64b_1C(
    TBitGrid        *bitGrid,
    TGridIndex      I,
    uint8_t         value
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
    uint8_t         value
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
    uint8_t         value
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
    unsigned int    nChannels,
    unsigned int    w,
    unsigned int    h
)
{
    TBitGrid        *newBitGrid = NULL;
    unsigned int    nBitsPerWord, nWordsTotal, nWordsPerRow;
    size_t          channelBytes, gridBytes;
    
    if ( nChannels > 8 || nChannels < 1 ) return NULL;
    
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
    uint8_t     value
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
////
//

TBitGridIterator*
TBitGridIteratorCreate(
    TBitGrid    *bitGrid,
    uint8_t     channelMask
)
{
    size_t              gridBytes = bitGrid->dimensions.nChannels * sizeof(TBitGridChannelPtr);
    TBitGridIterator    *iterator = NULL;
    unsigned int        nChannelsEnabled = 0;
    uint8_t             channelMaskCopy;
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
    uint8_t             value;
    unsigned int        i, j;
    
    // Test out a single-bit basic game board:
    gameBoard = TBitGridCreate(4, 10, 20);
    
    j = 0;
    while ( j < 20 ) {
        i = 0;
        while ( i < 10 ) {
            if ( ! (i % 2) || ! (j % 5) ) {
                uint8_t     v = 0x8 | (((i % 2) == 0) << 1) | ((j % 5) == 0) | ((random() % 2) << 2);
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
