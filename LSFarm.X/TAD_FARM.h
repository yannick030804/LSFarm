#ifndef TAD_FARM_H
#define TAD_FARM_H

#define FARM_STATE_SIZE 195

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
signed char Farm_GetSelectedAnimalIndex (void);
unsigned char Farm_IsRestRequestPending (void);
unsigned char Farm_IsRestFinished (void);
unsigned char Farm_IsRestSuccess (void);
void Farm_NotifyRestSuccess (void);
void Farm_NotifyRestTimeout (void);
unsigned char Farm_IsConfigured (void);
const char *Farm_GetName (void);
unsigned char Farm_GetCow (void);
unsigned char Farm_GetPig (void);
unsigned char Farm_GetHorse (void);
unsigned char Farm_GetChicken (void);
unsigned char Farm_GetNumCow (void);
unsigned char Farm_GetNumPig (void);
unsigned char Farm_GetNumHorse (void);
unsigned char Farm_GetNumChicken (void);
unsigned char Farm_GetMilk (void);
unsigned char Farm_GetHam (void);
unsigned char Farm_GetBrush (void);
unsigned char Farm_GetEggs (void);
unsigned char Farm_GetTotalAnimals (void);
unsigned char Farm_GetAnimalCount (void);
void Farm_GetAnimal (unsigned char index, unsigned char *species, unsigned char *number, unsigned char *critical);
unsigned char Farm_GetProductTotal (unsigned char species);
void Farm_SetRebellion (unsigned char active);
void Farm_Consume (unsigned char recipe);
unsigned char Farm_GetNotification (FarmNotification *notification);
void Farm_SetTimeReady (unsigned char ready);
void Farm_SetCurrentDate (unsigned char valid, unsigned char day, unsigned char month, unsigned char hour, unsigned char minute, unsigned char second);
void Farm_ExportState (unsigned char *buffer);
void Farm_ImportState (const unsigned char *buffer);
unsigned char Farm_IsDirty (void);
void Farm_ClearDirty (void);

#endif
