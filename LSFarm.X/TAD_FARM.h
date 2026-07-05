#ifndef TAD_FARM_H
#define TAD_FARM_H

#define FARM_STATE_SIZE 146

typedef struct {
    unsigned char kind;
    unsigned char species;
    unsigned char number;
} FarmNotification;

#define FARM_NOTIFICATION_ANIMAL  0
#define FARM_NOTIFICATION_PRODUCT 1

extern unsigned char configured;
extern unsigned char dirtyState;
extern char farmName[];
extern unsigned char productCounts[];
extern unsigned char totalAnimals;
extern unsigned char searchFinished;
extern unsigned char searchFound;
extern unsigned char restRequestPending;
extern unsigned char restFinished;
extern unsigned char restSuccess;

void Farm_Init (void);
void motorFarm (void);
void Farm_Reset (void);
void Farm_RequestConfigure (const char *name, unsigned char cow, unsigned char horse, unsigned char pig, unsigned char chicken);
void Farm_RequestSelectAnimal (unsigned char species, unsigned char number);
#define Farm_IsSearchFinished() (searchFinished)
#define Farm_IsAnimalFound() (searchFound)
#define Farm_IsRestRequestPending() (restRequestPending)
#define Farm_IsRestFinished() (restFinished)
#define Farm_IsRestSuccess() (restSuccess)
void Farm_NotifyRestSuccess (void);
void Farm_NotifyRestTimeout (void);
#define Farm_IsConfigured() (configured)
#define Farm_GetName() (farmName)
#define Farm_GetAnimalCount() (totalAnimals)
void Farm_GetAnimal (unsigned char index, unsigned char *species, unsigned char *number, unsigned char *critical);
#define Farm_GetProductTotal(species) (productCounts[(species)])
void Farm_SetRebellion (unsigned char active);
void Farm_Consume (unsigned char recipe);
unsigned char Farm_GetNotification (FarmNotification *notification);
void Farm_SetCurrentDate (unsigned char valid, unsigned char day, unsigned char month, unsigned char hour, unsigned char minute, unsigned char second);
unsigned char Farm_ExportByte (unsigned char index);
void Farm_BeginImportState (void);
void Farm_ImportStateByte (unsigned char index, unsigned char value);
void Farm_EndImportState (void);
#define Farm_IsDirty() (dirtyState)
#define Farm_ClearDirty() (dirtyState = 0)

#endif
