#ifndef TAD_FARM_H
#define TAD_FARM_H

#define FARM_STATE_SIZE 146
#define FARM_NAME_SIZE 17
#define FARM_NUM_SPECIES 4

typedef struct {
    unsigned char kind;
    unsigned char species;
    unsigned char number;
} FarmNotification;

#define FARM_NOTIFICATION_ANIMAL  0
#define FARM_NOTIFICATION_PRODUCT 1

extern unsigned char Farm_Configured;
extern char Farm_Name[FARM_NAME_SIZE];
extern unsigned char Farm_ProductCounts[FARM_NUM_SPECIES];
extern unsigned char Farm_TotalAnimals;
extern unsigned char Farm_SearchFinished;
extern unsigned char Farm_SearchFound;
extern unsigned char Farm_RestRequestPending;
extern unsigned char Farm_RestFinished;
extern unsigned char Farm_RestSuccess;
extern unsigned char Farm_DirtyState;

#define Farm_IsSearchFinished() (Farm_SearchFinished)
#define Farm_IsAnimalFound() (Farm_SearchFound)
#define Farm_IsRestRequestPending() (Farm_RestRequestPending)
#define Farm_IsRestFinished() (Farm_RestFinished)
#define Farm_IsRestSuccess() (Farm_RestSuccess)
#define Farm_IsConfigured() (Farm_Configured)
#define Farm_GetName() (Farm_Name)
#define Farm_GetAnimalCount() (Farm_TotalAnimals)
#define Farm_GetProductTotal(species) (Farm_ProductCounts[(species)])
#define Farm_IsDirty() (Farm_DirtyState)
#define Farm_ClearDirty() (Farm_DirtyState = 0)

void Farm_Init (void);
void motorFarm (void);
void Farm_Reset (void);
void Farm_RequestConfigure (const char *name, unsigned char cow, unsigned char horse, unsigned char pig, unsigned char chicken);
void Farm_RequestSelectAnimal (unsigned char species, unsigned char number);
void Farm_NotifyRestSuccess (void);
void Farm_NotifyRestTimeout (void);
void Farm_GetAnimal (unsigned char index, unsigned char *species, unsigned char *number, unsigned char *critical);
void Farm_SetRebellion (unsigned char active);
void Farm_Consume (unsigned char recipe);
unsigned char Farm_GetNotification (FarmNotification *notification);
void Farm_SetCurrentDate (unsigned char valid, unsigned char day, unsigned char month, unsigned char hour, unsigned char minute, unsigned char second);
unsigned char Farm_ExportByte (unsigned char index);
void Farm_BeginImportState (void);
void Farm_ImportStateByte (unsigned char index, unsigned char value);
void Farm_EndImportState (void);

#endif
