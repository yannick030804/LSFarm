#include <xc.h>
#include "TAD_FARM.h"
#include "TAD_TIMER.h"

#define FARM_MAX_NAME 16
#define FARM_MAX_ANIMALS 24
#define FARM_NUM_SPECIES 4
#define FARM_MIN_ANIMALS_PER_SPECIES 3

#define SPECIES_COW 0
#define SPECIES_PIG 1
#define SPECIES_HORSE 2
#define SPECIES_CHICKEN 3

#define CRITICAL_TIME 120000UL
#define FARM_NOTIFICATION_QUEUE_SIZE 8
#define ANIMAL_SPECIES_MASK 0x03
#define ANIMAL_CRITICAL_MASK 0x80
#define YEAR_SECONDS 31536000UL
#define FARM_NOTIFICATION_MASK (FARM_NOTIFICATION_QUEUE_SIZE - 1)
#define PENDING_TIME(species) pendingTimes[(species)]
#define GENERATION_TIME(species) generationTimes[(species)]
#define ANIMAL_COUNT(species) animalCounts[(species)]
#define CRITICAL_COUNT(species) criticalCounts[(species)]
#define PRODUCT_COUNT(species) productCounts[(species)]
#define LAST_GENERATION(species) lastGeneration[(species)]
#define LAST_PRODUCT(species) lastProduct[(species)]
#define ANIMAL_SPECIES(index) ((unsigned char)(animalInfo[(index)] & ANIMAL_SPECIES_MASK))
#define ANIMAL_IS_CRITICAL(index) ((unsigned char)((animalInfo[(index)] & ANIMAL_CRITICAL_MASK) != 0))

static unsigned char timerHandle;
unsigned char configured;
static unsigned char rebellion;
static unsigned char resetRequested;
unsigned char dirtyState;
static unsigned char currentDateValid;
static unsigned long currentStamp;

char farmName[FARM_MAX_NAME + 1];
static const char *pendingName;
static unsigned char pendingTimes[FARM_NUM_SPECIES];
static unsigned char configRequested;

static unsigned char generationTimes[FARM_NUM_SPECIES];
static unsigned char animalCounts[FARM_NUM_SPECIES];
static unsigned char criticalCounts[FARM_NUM_SPECIES];
unsigned char productCounts[FARM_NUM_SPECIES];

unsigned char totalAnimals;

static unsigned char selectedAnimalSpecies;
static unsigned char selectedAnimalNumber;
static signed char selectedAnimalIndex;
static unsigned char searchRequested;
unsigned char searchFinished;
unsigned char searchFound;
unsigned char restRequestPending;
unsigned char restFinished;
unsigned char restSuccess;

static unsigned char lastGeneration[FARM_NUM_SPECIES];
static unsigned char lastProduct[FARM_NUM_SPECIES];

static unsigned char animalInfo[FARM_MAX_ANIMALS];
static unsigned long animalSleepStamp[FARM_MAX_ANIMALS];
static unsigned char notificationMeta[FARM_NOTIFICATION_QUEUE_SIZE];
static unsigned char notificationValues[FARM_NOTIFICATION_QUEUE_SIZE];
static unsigned char notificationHead;
static unsigned char notificationCount;

static void resetFarmData (void);
static void Farm_ClearCurrentData (void);
static void Farm_RecountAnimals (void);
static void Farm_PushNotification (unsigned char kind, unsigned char species, unsigned char number);
static unsigned char Farm_CreateAnimal (unsigned char species);
static void Farm_TakeProduct (unsigned char species, unsigned char amount);
static void Farm_ProcessGeneration (unsigned char species, unsigned char now);
static void Farm_ProcessProducts (unsigned char species, unsigned char now);
static void Farm_ProcessCriticalAnimal (unsigned char index);
static void Farm_ResetSelectionState (void);
static unsigned long Farm_BuildDateSeconds (unsigned char day, unsigned char month, unsigned char hour, unsigned char minute, unsigned char second);
static unsigned char Farm_GetNowSeconds (void);
static unsigned long Farm_GetSleepElapsedSeconds (unsigned char index);
static void Farm_ClearSpeciesData (unsigned char clearGenerationTimes);
static unsigned char Farm_ExportAnimalByte (unsigned char index);
static void Farm_ImportAnimalByte (unsigned char index, unsigned char value);

void Farm_Init (void) {
    TI_NewTimer(&timerHandle);
    resetFarmData();
    TI_ResetTics(timerHandle);
}

void motorFarm (void) {
    static unsigned char state = 0;
    static unsigned char copyIndex = 0;
    static unsigned char speciesIndex = 0;
    static unsigned char animalIndex = 0;
    static unsigned char speciesNumber = 0;
    static unsigned char now = 0;

    if (resetRequested == 1) {
        state = 0;
        copyIndex = 0;
        speciesIndex = 0;
        animalIndex = 0;
        speciesNumber = 0;
        now = 0;
        resetRequested = 0;
    }

    switch (state) {
        case 0:
            if (configRequested == 1) {
                copyIndex = 0;
                state++;
            }
            break;

        case 1:
            if (pendingName[copyIndex] != '\0' && copyIndex < sizeof(farmName) - 1) {
                farmName[copyIndex] = pendingName[copyIndex];
                copyIndex++;
            } else {
                farmName[copyIndex] = '\0';
                state++;
            }
            break;

        case 2:
            Farm_ClearCurrentData();
            GENERATION_TIME(SPECIES_COW) = PENDING_TIME(SPECIES_COW);
            GENERATION_TIME(SPECIES_PIG) = PENDING_TIME(SPECIES_PIG);
            GENERATION_TIME(SPECIES_HORSE) = PENDING_TIME(SPECIES_HORSE);
            GENERATION_TIME(SPECIES_CHICKEN) = PENDING_TIME(SPECIES_CHICKEN);
            configured = 1;
            configRequested = 0;
            dirtyState = 1;
            TI_ResetTics(timerHandle);
            state++;
            break;

        case 3:
            if (configured == 0 || currentDateValid == 0) {
                break;
            }
            speciesIndex = 0;
            state++;
            break;

        case 4:
            now = Farm_GetNowSeconds();
            if (speciesIndex < FARM_NUM_SPECIES) {
                Farm_ProcessGeneration(speciesIndex, now);
                speciesIndex++;
            } else {
                speciesIndex = 0;
                state++;
            }
            break;

        case 5:
            now = Farm_GetNowSeconds();
            if (speciesIndex < FARM_NUM_SPECIES) {
                Farm_ProcessProducts(speciesIndex, now);
                speciesIndex++;
            } else {
                animalIndex = 0;
                state++;
            }
            break;

        case 6:
            if (animalIndex < totalAnimals) {
                Farm_ProcessCriticalAnimal(animalIndex);
                animalIndex++;
            } else if (searchRequested == 1) {
                animalIndex = 0;
                speciesNumber = 0;
                searchFinished = 0;
                searchFound = 0;
                selectedAnimalIndex = -1;
                state++;
            } else {
                state = 3;
            }
            break;

        case 7:
            if (animalIndex < totalAnimals) {
                if (ANIMAL_SPECIES(animalIndex) == selectedAnimalSpecies) {
                    speciesNumber++;
                }
                if (ANIMAL_SPECIES(animalIndex) == selectedAnimalSpecies && speciesNumber == selectedAnimalNumber) {
                    selectedAnimalIndex = (signed char)animalIndex;
                    searchFound = 1;
                    searchFinished = 1;
                    restRequestPending = 1;
                    restFinished = 0;
                    restSuccess = 0;
                    searchRequested = 0;
                    state = 3;
                } else {
                    animalIndex++;
                }
            } else {
                searchFinished = 1;
                searchRequested = 0;
                state = 3;
            }
            break;
    }
}

void Farm_Reset (void) {
    resetFarmData();
    TI_ResetTics(timerHandle);
    resetRequested = 1;
}

void Farm_RequestConfigure (const char *name, unsigned char cow, unsigned char horse, unsigned char pig, unsigned char chicken) {
    pendingName = name;
    PENDING_TIME(SPECIES_COW) = cow;
    PENDING_TIME(SPECIES_HORSE) = horse;
    PENDING_TIME(SPECIES_PIG) = pig;
    PENDING_TIME(SPECIES_CHICKEN) = chicken;
    Farm_ResetSelectionState();
    configRequested = 1;
    configured = 0;
}

void Farm_RequestSelectAnimal (unsigned char species, unsigned char number) {
    Farm_ResetSelectionState();
    selectedAnimalSpecies = species;
    selectedAnimalNumber = number;
    selectedAnimalIndex = -1;
    searchRequested = 1;
}

void Farm_NotifyRestSuccess (void) {
    unsigned char index;
    unsigned char species;

    if (selectedAnimalIndex < 0) {
        restRequestPending = 0;
        restFinished = 1;
        restSuccess = 0;
        return;
    }

    index = (unsigned char)selectedAnimalIndex;
    species = ANIMAL_SPECIES(index);

    if (ANIMAL_IS_CRITICAL(index) == 1) {
        animalInfo[index] &= (unsigned char)(~ANIMAL_CRITICAL_MASK);
        if (CRITICAL_COUNT(species) > 0) {
            CRITICAL_COUNT(species)--;
        }
    }

    animalSleepStamp[index] = currentStamp;
    dirtyState = 1;
    Farm_ResetSelectionState();
    restFinished = 1;
    restSuccess = 1;
}

void Farm_NotifyRestTimeout (void) {
    Farm_ResetSelectionState();
    restFinished = 1;
}

void Farm_GetAnimal (unsigned char index, unsigned char *species, unsigned char *number, unsigned char *critical) {
    unsigned char i;
    unsigned char count = 0;

    *species = ANIMAL_SPECIES(index);
    *critical = ANIMAL_IS_CRITICAL(index);

    for (i = 0; i <= index; i++) {
        if (ANIMAL_SPECIES(i) == *species) {
            count++;
        }
    }

    *number = count;
}

void Farm_SetRebellion (unsigned char active) {
    rebellion = active;
}

void Farm_Consume (unsigned char recipe) {
    switch (recipe) {
        case 0:
            Farm_TakeProduct(SPECIES_CHICKEN, 1);
            break;
        case 1:
            Farm_TakeProduct(SPECIES_CHICKEN, 1);
            Farm_TakeProduct(SPECIES_PIG, 1);
            break;
        case 2:
            Farm_TakeProduct(SPECIES_COW, 2);
            break;
        case 3:
            Farm_TakeProduct(SPECIES_HORSE, 2);
            break;
    }
    dirtyState = 1;
}

unsigned char Farm_GetNotification (FarmNotification *notification) {
    if (notificationCount == 0) {
        return 0;
    }

    notification->kind = (unsigned char)(notificationMeta[notificationHead] >> 2);
    notification->species = (unsigned char)(notificationMeta[notificationHead] & 0x03);
    notification->number = notificationValues[notificationHead];
    notificationHead = (unsigned char)((notificationHead + 1) & FARM_NOTIFICATION_MASK);
    notificationCount--;
    return 1;
}

void Farm_SetCurrentDate (unsigned char valid, unsigned char day, unsigned char month, unsigned char hour, unsigned char minute, unsigned char second) {
    currentDateValid = valid;
    if (valid == 1) {
        currentStamp = Farm_BuildDateSeconds(day, month, hour, minute, second);
    } else {
        currentStamp = 0;
    }
}

unsigned char Farm_ExportByte (unsigned char index) {
    if (index < (FARM_MAX_NAME + 1)) {
        return (unsigned char)farmName[index];
    }

    index = (unsigned char)(index - (FARM_MAX_NAME + 1));
    if (index < FARM_NUM_SPECIES) {
        return GENERATION_TIME(index);
    }

    index = (unsigned char)(index - FARM_NUM_SPECIES);
    if (index == 0) {
        return totalAnimals;
    }

    index--;
    if (index < FARM_NUM_SPECIES) {
        return PRODUCT_COUNT(index);
    }

    index = (unsigned char)(index - FARM_NUM_SPECIES);
    return Farm_ExportAnimalByte(index);
}

void Farm_BeginImportState (void) {
    Farm_ClearCurrentData();
}

void Farm_ImportStateByte (unsigned char index, unsigned char value) {
    if (index < (FARM_MAX_NAME + 1)) {
        farmName[index] = (char)value;
        return;
    }

    index = (unsigned char)(index - (FARM_MAX_NAME + 1));
    if (index < FARM_NUM_SPECIES) {
        GENERATION_TIME(index) = value;
        return;
    }

    index = (unsigned char)(index - FARM_NUM_SPECIES);
    if (index == 0) {
        totalAnimals = value;
        return;
    }

    index--;
    if (index < FARM_NUM_SPECIES) {
        PRODUCT_COUNT(index) = value;
        return;
    }

    index = (unsigned char)(index - FARM_NUM_SPECIES);
    Farm_ImportAnimalByte(index, value);
}

void Farm_EndImportState (void) {
    unsigned char i;

    if (totalAnimals > FARM_MAX_ANIMALS) {
        totalAnimals = FARM_MAX_ANIMALS;
    }

    farmName[FARM_MAX_NAME] = '\0';
    for (i = 0; i < totalAnimals; i++) {
        if (ANIMAL_SPECIES(i) >= FARM_NUM_SPECIES) {
            animalInfo[i] = (unsigned char)((animalInfo[i] & ANIMAL_CRITICAL_MASK) | SPECIES_COW);
        }
        animalInfo[i] &= (unsigned char)(~ANIMAL_CRITICAL_MASK);
    }

    Farm_RecountAnimals();
    configured = 1;
    dirtyState = 0;
}

static void resetFarmData (void) {
    configured = 0;
    rebellion = 0;
    resetRequested = 0;
    dirtyState = 0;
    currentDateValid = 0;
    currentStamp = 0;
    farmName[0] = '\0';
    pendingName = 0;
    configRequested = 0;
    totalAnimals = 0;
    notificationHead = 0;
    notificationCount = 0;
    Farm_ResetSelectionState();

    PENDING_TIME(SPECIES_COW) = 0;
    PENDING_TIME(SPECIES_PIG) = 0;
    PENDING_TIME(SPECIES_HORSE) = 0;
    PENDING_TIME(SPECIES_CHICKEN) = 0;
    Farm_ClearSpeciesData(1);
}

static void Farm_ClearCurrentData (void) {
    totalAnimals = 0;
    notificationHead = 0;
    notificationCount = 0;
    rebellion = 0;
    dirtyState = 0;
    Farm_ResetSelectionState();
    Farm_ClearSpeciesData(0);
}

static void Farm_RecountAnimals (void) {
    unsigned char i;

    for (i = 0; i < FARM_NUM_SPECIES; i++) {
        ANIMAL_COUNT(i) = 0;
        CRITICAL_COUNT(i) = 0;
    }

    for (i = 0; i < totalAnimals; i++) {
        ANIMAL_COUNT(ANIMAL_SPECIES(i))++;
        if (ANIMAL_IS_CRITICAL(i) == 1) {
            CRITICAL_COUNT(ANIMAL_SPECIES(i))++;
        }
    }
}

static void Farm_PushNotification (unsigned char kind, unsigned char species, unsigned char number) {
    unsigned char tail;

    if (notificationCount >= FARM_NOTIFICATION_QUEUE_SIZE) {
        return;
    }

    tail = (unsigned char)((notificationHead + notificationCount) & FARM_NOTIFICATION_MASK);

    notificationMeta[tail] = (unsigned char)(((kind & 0x3F) << 2) | (species & 0x03));
    notificationValues[tail] = number;
    notificationCount++;
}

static unsigned char Farm_CreateAnimal (unsigned char species) {
    unsigned char missingAnimals = 0;
    unsigned char i;

    if (totalAnimals >= FARM_MAX_ANIMALS) {
        return 0;
    }

    for (i = 0; i < FARM_NUM_SPECIES; i++) {
        unsigned char count = ANIMAL_COUNT(i);
        if (i == species) {
            count++;
        }
        if (count < FARM_MIN_ANIMALS_PER_SPECIES) {
            missingAnimals += (unsigned char)(FARM_MIN_ANIMALS_PER_SPECIES - count);
        }
    }

    if ((unsigned char)(totalAnimals + 1 + missingAnimals) > FARM_MAX_ANIMALS) {
        return 0;
    }

    animalInfo[totalAnimals] = (unsigned char)(species & ANIMAL_SPECIES_MASK);
    animalSleepStamp[totalAnimals] = currentStamp;
    totalAnimals++;
    return 1;
}

static void Farm_TakeProduct (unsigned char species, unsigned char amount) {
    if (PRODUCT_COUNT(species) >= amount) {
        PRODUCT_COUNT(species) = (unsigned char)(PRODUCT_COUNT(species) - amount);
    }
}

static void Farm_ProcessGeneration (unsigned char species, unsigned char now) {
    if (GENERATION_TIME(species) == 0) {
        return;
    }

    if ((unsigned char)(now - LAST_GENERATION(species)) >= GENERATION_TIME(species)) {
        if (Farm_CreateAnimal(species) == 1) {
            ANIMAL_COUNT(species)++;
            Farm_PushNotification(FARM_NOTIFICATION_ANIMAL, species, ANIMAL_COUNT(species));
            dirtyState = 1;
        }
        LAST_GENERATION(species) = now;
    }
}

static void Farm_ProcessProducts (unsigned char species, unsigned char now) {
    unsigned char awakeCount;
    unsigned char totalProducts;
    unsigned char productTime;

    if (rebellion == 1) {
        return;
    }

    if (species == SPECIES_COW) {
        productTime = 47;
    } else if (species == SPECIES_PIG) {
        productTime = 31;
    } else if (species == SPECIES_HORSE) {
        productTime = 23;
    } else {
        productTime = 13;
    }

    if ((unsigned char)(now - LAST_PRODUCT(species)) < productTime) {
        return;
    }

    awakeCount = (unsigned char)(ANIMAL_COUNT(species) - CRITICAL_COUNT(species));
    if (awakeCount > 0) {
        totalProducts = PRODUCT_COUNT(species);
        if (awakeCount > (unsigned char)(255 - totalProducts)) {
            PRODUCT_COUNT(species) = 255;
        } else {
            PRODUCT_COUNT(species) = (unsigned char)(totalProducts + awakeCount);
        }
        Farm_PushNotification(FARM_NOTIFICATION_PRODUCT, species, PRODUCT_COUNT(species));
        dirtyState = 1;
    }
    LAST_PRODUCT(species) = now;
}

static void Farm_ProcessCriticalAnimal (unsigned char index) {
    unsigned char species;

    if (ANIMAL_IS_CRITICAL(index) == 1) {
        return;
    }

    if (currentDateValid == 0) {
        return;
    }

    if (Farm_GetSleepElapsedSeconds(index) < 120UL) {
        return;
    }

    species = ANIMAL_SPECIES(index);
    animalInfo[index] |= ANIMAL_CRITICAL_MASK;
    CRITICAL_COUNT(species)++;
    dirtyState = 1;
}

static void Farm_ResetSelectionState (void) {
    selectedAnimalSpecies = 0;
    selectedAnimalNumber = 0;
    selectedAnimalIndex = -1;
    searchRequested = 0;
    searchFinished = 0;
    searchFound = 0;
    restRequestPending = 0;
    restFinished = 0;
    restSuccess = 0;
}

static unsigned long Farm_BuildDateSeconds (unsigned char day, unsigned char month, unsigned char hour, unsigned char minute, unsigned char second) {
    unsigned int monthOffset = 0;
    unsigned int dayOffset;
    unsigned long total = second;

    if (month == 0) {
        month = 1;
    }
    if (day == 0) {
        day = 1;
    }
    if (month > 12) {
        month = 12;
    }

    if (month > 1) monthOffset += 31;
    if (month > 2) monthOffset += 28;
    if (month > 3) monthOffset += 31;
    if (month > 4) monthOffset += 30;
    if (month > 5) monthOffset += 31;
    if (month > 6) monthOffset += 30;
    if (month > 7) monthOffset += 31;
    if (month > 8) monthOffset += 31;
    if (month > 9) monthOffset += 30;
    if (month > 10) monthOffset += 31;
    if (month > 11) monthOffset += 30;

    while (minute > 0) {
        total += 60UL;
        minute--;
    }

    while (hour > 0) {
        total += 3600UL;
        hour--;
    }

    dayOffset = (unsigned int)(monthOffset + (unsigned int)(day - 1));
    while (dayOffset > 0) {
        total += 86400UL;
        dayOffset--;
    }

    return total;
}

static unsigned char Farm_GetNowSeconds (void) {
    unsigned long tics = TI_GetTics(timerHandle);
    unsigned char seconds = 0;

    while (tics >= 1000UL) {
        tics -= 1000UL;
        seconds++;
    }

    return seconds;
}

static unsigned long Farm_GetSleepElapsedSeconds (unsigned char index) {
    unsigned long sleepTotal;

    sleepTotal = animalSleepStamp[index];

    if (currentStamp >= sleepTotal) {
        return currentStamp - sleepTotal;
    }

    return (YEAR_SECONDS - sleepTotal) + currentStamp;
}

static void Farm_ClearSpeciesData (unsigned char clearGenerationTimes) {
    unsigned char i;

    for (i = 0; i < FARM_NUM_SPECIES; i++) {
        ANIMAL_COUNT(i) = 0;
        CRITICAL_COUNT(i) = 0;
        PRODUCT_COUNT(i) = 0;
        LAST_GENERATION(i) = 0;
        LAST_PRODUCT(i) = 0;
        if (clearGenerationTimes == 1) {
            GENERATION_TIME(i) = 0;
        }
    }
}

static unsigned char Farm_ExportAnimalByte (unsigned char index) {
    unsigned char animalIndex = 0;
    unsigned char *stampBytes;

    while (index >= 5) {
        index = (unsigned char)(index - 5);
        animalIndex++;
    }

    if (animalIndex >= FARM_MAX_ANIMALS || animalIndex >= totalAnimals) {
        return 0;
    }

    if (index == 0) {
        return animalInfo[animalIndex];
    }

    stampBytes = (unsigned char *)&animalSleepStamp[animalIndex];
    return stampBytes[index - 1];
}

static void Farm_ImportAnimalByte (unsigned char index, unsigned char value) {
    unsigned char animalIndex = 0;
    unsigned char *stampBytes;

    while (index >= 5) {
        index = (unsigned char)(index - 5);
        animalIndex++;
    }

    if (animalIndex >= FARM_MAX_ANIMALS) {
        return;
    }

    if (index == 0) {
        animalInfo[animalIndex] = value;
        return;
    }

    stampBytes = (unsigned char *)&animalSleepStamp[animalIndex];
    stampBytes[index - 1] = value;
}
