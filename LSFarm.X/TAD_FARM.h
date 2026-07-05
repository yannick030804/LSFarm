#ifndef TAD_FARM_H
#define TAD_FARM_H

typedef struct {
    unsigned char kind;
    unsigned char species;
    unsigned char number;
} FarmNotification;

#define FARM_NOTIFICATION_ANIMAL  0
#define FARM_NOTIFICATION_PRODUCT 1

void Farm_Init (void);
void motorFarm (void);
void Farm_Reset (void);
void Farm_RequestConfigure (const char *name, unsigned char cow, unsigned char horse, unsigned char pig, unsigned char chicken);
void Farm_RequestSelectAnimal (unsigned char species, unsigned char number);
unsigned char Farm_IsSearchFinished (void);
unsigned char Farm_IsAnimalFound (void);
unsigned char Farm_IsRestRequestPending (void);
unsigned char Farm_IsRestFinished (void);
unsigned char Farm_IsRestSuccess (void);
void Farm_NotifyRestSuccess (void);
void Farm_NotifyRestTimeout (void);
unsigned char Farm_IsConfigured (void);
const char *Farm_GetName (void);
unsigned char Farm_GetAnimalCount (void);
void Farm_GetAnimal (unsigned char index, unsigned char *species, unsigned char *number, unsigned char *critical);
unsigned char Farm_GetProductTotal (unsigned char species);
void Farm_SetRebellion (unsigned char active);
void Farm_Consume (unsigned char recipe);
unsigned char Farm_GetNotification (FarmNotification *notification);
void Farm_SetCurrentDate (unsigned char valid, unsigned char day, unsigned char month, unsigned char hour, unsigned char minute, unsigned char second);

#endif
