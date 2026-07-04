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

static unsigned char timerHandle;
static unsigned char configured;
static unsigned char rebellion;
static unsigned char resetRequested;
static unsigned char dirtyState;
static unsigned char currentDateValid;
static unsigned char currentDay;
static unsigned char currentMonth;
static unsigned char currentHour;
static unsigned char currentMinute;
static unsigned char currentSecond;

static char farmName[FARM_MAX_NAME + 1];
static const char *pendingName;
static unsigned char pendingTimeCow;
static unsigned char pendingTimePig;
static unsigned char pendingTimeHorse;
static unsigned char pendingTimeChicken;
static unsigned char configRequested;

static unsigned char generationTimeCow;
static unsigned char generationTimePig;
static unsigned char generationTimeHorse;
static unsigned char generationTimeChicken;
static unsigned char animalCountCow;
static unsigned char animalCountPig;
static unsigned char animalCountHorse;
static unsigned char animalCountChicken;
static unsigned char criticalCountCow;
static unsigned char criticalCountPig;
static unsigned char criticalCountHorse;
static unsigned char criticalCountChicken;
static unsigned char productCountCow;
static unsigned char productCountPig;
static unsigned char productCountHorse;
static unsigned char productCountChicken;

static unsigned char totalAnimals;

static unsigned char selectedAnimalSpecies;
static unsigned char selectedAnimalNumber;
static signed char selectedAnimalIndex;
static unsigned char searchRequested;
static unsigned char searchFinished;
static unsigned char searchFound;
static unsigned char restRequestPending;
static unsigned char restFinished;
static unsigned char restSuccess;

static unsigned char lastGenerationCow;
static unsigned char lastGenerationPig;
static unsigned char lastGenerationHorse;
static unsigned char lastGenerationChicken;
static unsigned char lastProductCow;
static unsigned char lastProductPig;
static unsigned char lastProductHorse;
static unsigned char lastProductChicken;

static unsigned char animalInfo[FARM_MAX_ANIMALS];
static unsigned long animalSleepStamp[FARM_MAX_ANIMALS];
static unsigned char notificationMeta[FARM_NOTIFICATION_QUEUE_SIZE];
static unsigned char notificationValueA[4];
static unsigned char notificationValueB[4];
static unsigned char notificationHead;
static unsigned char notificationCount;

static void resetFarmData (void);
static void Farm_ClearCurrentData (void);
static void Farm_RecountAnimals (void);
static void Farm_MarkDirty (void);
static void Farm_PushNotification (unsigned char kind, unsigned char species, unsigned char number);
static unsigned char Farm_CreateAnimal (unsigned char species);
static unsigned char Farm_GetAwakeCount (unsigned char species);
static void Farm_TakeProduct (unsigned char species, unsigned char amount);
static void Farm_ProcessGeneration (unsigned char species, unsigned char now);
static void Farm_ProcessProducts (unsigned char species, unsigned char now);
static void Farm_ProcessCriticalAnimal (unsigned char index);
static void Farm_ResetSelectionState (void);
static unsigned long Farm_BuildDateSeconds (unsigned char day, unsigned char month, unsigned char hour, unsigned char minute, unsigned char second);
static unsigned long Farm_GetCurrentDateStamp (void);
static unsigned long Farm_GetSleepElapsedSeconds (unsigned char index);
static void Farm_StoreSleepDate (unsigned char index);
static unsigned char Farm_GetAnimalSpecies (unsigned char index);
static unsigned char Farm_IsAnimalCritical (unsigned char index);
static void Farm_SetAnimalSpecies (unsigned char index, unsigned char species);
static void Farm_SetAnimalCritical (unsigned char index, unsigned char critical);
static void Farm_SetNotificationValue (unsigned char index, unsigned char value);
static unsigned char Farm_GetNotificationValue (unsigned char index);
static unsigned char Farm_GetPendingTime (unsigned char species);
static void Farm_SetPendingTime (unsigned char species, unsigned char value);
static unsigned char Farm_GetGenerationTimeBySpecies (unsigned char species);
static void Farm_SetGenerationTimeBySpecies (unsigned char species, unsigned char value);
static unsigned char Farm_GetAnimalCountBySpecies (unsigned char species);
static void Farm_SetAnimalCountBySpecies (unsigned char species, unsigned char value);
static unsigned char Farm_GetCriticalCountBySpecies (unsigned char species);
static void Farm_SetCriticalCountBySpecies (unsigned char species, unsigned char value);
static unsigned char Farm_GetProductCountBySpecies (unsigned char species);
static void Farm_SetProductCountBySpecies (unsigned char species, unsigned char value);
static unsigned char Farm_GetLastGenerationBySpecies (unsigned char species);
static void Farm_SetLastGenerationBySpecies (unsigned char species, unsigned char value);
static unsigned char Farm_GetLastProductBySpecies (unsigned char species);
static void Farm_SetLastProductBySpecies (unsigned char species, unsigned char value);
static unsigned char Farm_GetProductTimeBySpecies (unsigned char species);

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
                state = 1;
            }
            break;

        case 1:
            if (pendingName[copyIndex] != '\0' && copyIndex < sizeof(farmName) - 1) {
                farmName[copyIndex] = pendingName[copyIndex];
                copyIndex++;
            } else {
                farmName[copyIndex] = '\0';
                state = 2;
            }
            break;

        case 2:
            Farm_ClearCurrentData();
            Farm_SetGenerationTimeBySpecies(SPECIES_COW, Farm_GetPendingTime(SPECIES_COW));
            Farm_SetGenerationTimeBySpecies(SPECIES_PIG, Farm_GetPendingTime(SPECIES_PIG));
            Farm_SetGenerationTimeBySpecies(SPECIES_HORSE, Farm_GetPendingTime(SPECIES_HORSE));
            Farm_SetGenerationTimeBySpecies(SPECIES_CHICKEN, Farm_GetPendingTime(SPECIES_CHICKEN));
            configured = 1;
            configRequested = 0;
            Farm_MarkDirty();
            TI_ResetTics(timerHandle);
            state = 3;
            break;

        case 3:
            if (configured == 0 || currentDateValid == 0) {
                break;
            }
            speciesIndex = 0;
            state = 4;
            break;

        case 4:
            now = (unsigned char)(TI_GetTics(timerHandle) / 1000UL);
            if (speciesIndex < FARM_NUM_SPECIES) {
                Farm_ProcessGeneration(speciesIndex, now);
                speciesIndex++;
            } else {
                speciesIndex = 0;
                state = 5;
            }
            break;

        case 5:
            now = (unsigned char)(TI_GetTics(timerHandle) / 1000UL);
            if (speciesIndex < FARM_NUM_SPECIES) {
                Farm_ProcessProducts(speciesIndex, now);
                speciesIndex++;
            } else {
                animalIndex = 0;
                state = 6;
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
                state = 7;
            } else {
                state = 3;
            }
            break;

        case 7:
            if (animalIndex < totalAnimals) {
                if (Farm_GetAnimalSpecies(animalIndex) == selectedAnimalSpecies) {
                    speciesNumber++;
                }
                if (Farm_GetAnimalSpecies(animalIndex) == selectedAnimalSpecies && speciesNumber == selectedAnimalNumber) {
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
    Farm_SetPendingTime(SPECIES_COW, cow);
    Farm_SetPendingTime(SPECIES_HORSE, horse);
    Farm_SetPendingTime(SPECIES_PIG, pig);
    Farm_SetPendingTime(SPECIES_CHICKEN, chicken);
    Farm_ResetSelectionState();
    configRequested = 1;
    configured = 0;
}

void Farm_RequestSelectAnimal (unsigned char species, unsigned char number) {
    selectedAnimalSpecies = species;
    selectedAnimalNumber = number;
    selectedAnimalIndex = -1;
    searchRequested = 1;
    searchFinished = 0;
    searchFound = 0;
    restRequestPending = 0;
    restFinished = 0;
    restSuccess = 0;
}

unsigned char Farm_IsSearchFinished (void) {
    return searchFinished;
}

unsigned char Farm_IsAnimalFound (void) {
    return searchFound;
}

signed char Farm_GetSelectedAnimalIndex (void) {
    return selectedAnimalIndex;
}

unsigned char Farm_IsRestRequestPending (void) {
    return restRequestPending;
}

unsigned char Farm_IsRestFinished (void) {
    return restFinished;
}

unsigned char Farm_IsRestSuccess (void) {
    return restSuccess;
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
    species = Farm_GetAnimalSpecies(index);

    if (Farm_IsAnimalCritical(index) == 1) {
        Farm_SetAnimalCritical(index, 0);
        if (Farm_GetCriticalCountBySpecies(species) > 0) {
            Farm_SetCriticalCountBySpecies(species, (unsigned char)(Farm_GetCriticalCountBySpecies(species) - 1));
        }
    }

    Farm_StoreSleepDate(index);
    restRequestPending = 0;
    restFinished = 1;
    restSuccess = 1;
    Farm_MarkDirty();
    selectedAnimalSpecies = 0;
    selectedAnimalNumber = 0;
    selectedAnimalIndex = -1;
}

void Farm_NotifyRestTimeout (void) {
    restRequestPending = 0;
    restFinished = 1;
    restSuccess = 0;
    selectedAnimalSpecies = 0;
    selectedAnimalNumber = 0;
    selectedAnimalIndex = -1;
}

unsigned char Farm_IsConfigured (void) {
    return configured;
}

const char *Farm_GetName (void) {
    return farmName;
}

unsigned char Farm_GetCow (void) {
    return Farm_GetGenerationTimeBySpecies(SPECIES_COW);
}

unsigned char Farm_GetPig (void) {
    return Farm_GetGenerationTimeBySpecies(SPECIES_PIG);
}

unsigned char Farm_GetHorse (void) {
    return Farm_GetGenerationTimeBySpecies(SPECIES_HORSE);
}

unsigned char Farm_GetChicken (void) {
    return Farm_GetGenerationTimeBySpecies(SPECIES_CHICKEN);
}

unsigned char Farm_GetNumCow (void) {
    return Farm_GetAnimalCountBySpecies(SPECIES_COW);
}

unsigned char Farm_GetNumPig (void) {
    return Farm_GetAnimalCountBySpecies(SPECIES_PIG);
}

unsigned char Farm_GetNumHorse (void) {
    return Farm_GetAnimalCountBySpecies(SPECIES_HORSE);
}

unsigned char Farm_GetNumChicken (void) {
    return Farm_GetAnimalCountBySpecies(SPECIES_CHICKEN);
}

unsigned char Farm_GetMilk (void) {
    return Farm_GetProductCountBySpecies(SPECIES_COW);
}

unsigned char Farm_GetHam (void) {
    return Farm_GetProductCountBySpecies(SPECIES_PIG);
}

unsigned char Farm_GetBrush (void) {
    return Farm_GetProductCountBySpecies(SPECIES_HORSE);
}

unsigned char Farm_GetEggs (void) {
    return Farm_GetProductCountBySpecies(SPECIES_CHICKEN);
}

unsigned char Farm_GetTotalAnimals (void) {
    return totalAnimals;
}

unsigned char Farm_GetAnimalCount (void) {
    return totalAnimals;
}

void Farm_GetAnimal (unsigned char index, unsigned char *species, unsigned char *number, unsigned char *critical) {
    unsigned char i;
    unsigned char count = 0;

    *species = Farm_GetAnimalSpecies(index);
    *critical = Farm_IsAnimalCritical(index);

    for (i = 0; i <= index; i++) {
        if (Farm_GetAnimalSpecies(i) == *species) {
            count++;
        }
    }

    *number = count;
}

unsigned char Farm_GetProductTotal (unsigned char species) {
    return Farm_GetProductCountBySpecies(species);
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
    Farm_MarkDirty();
}

unsigned char Farm_GetNotification (FarmNotification *notification) {
    if (notificationCount == 0) {
        return 0;
    }

    notification->kind = (unsigned char)(notificationMeta[notificationHead] >> 2);
    notification->species = (unsigned char)(notificationMeta[notificationHead] & 0x03);
    notification->number = Farm_GetNotificationValue(notificationHead);
    notificationHead++;
    if (notificationHead >= FARM_NOTIFICATION_QUEUE_SIZE) {
        notificationHead = 0;
    }
    notificationCount--;
    return 1;
}

void Farm_SetTimeReady (unsigned char ready) {
    if (ready == 0) {
        currentDateValid = 0;
    }
}

void Farm_SetCurrentDate (unsigned char valid, unsigned char day, unsigned char month, unsigned char hour, unsigned char minute, unsigned char second) {
    currentDateValid = valid;
    if (valid == 1) {
        currentDay = day;
        currentMonth = month;
        currentHour = hour;
        currentMinute = minute;
        currentSecond = second;
    }
}

void Farm_ExportState (unsigned char *buffer) {
    unsigned char i;
    unsigned char index = 0;

    for (i = 0; i < sizeof(farmName); i++) {
        buffer[index] = (unsigned char)farmName[i];
        index++;
    }

    for (i = 0; i < FARM_NUM_SPECIES; i++) {
        buffer[index] = Farm_GetGenerationTimeBySpecies(i);
        index++;
    }

    buffer[index] = totalAnimals;
    index++;
    for (i = 0; i < FARM_NUM_SPECIES; i++) {
        buffer[index] = Farm_GetProductCountBySpecies(i);
        index++;
    }

    for (i = 0; i < FARM_MAX_ANIMALS; i++) {
        if (i < totalAnimals) {
            buffer[index] = animalInfo[i];
            index++;
            buffer[index] = (unsigned char)(animalSleepStamp[i] & 0xFFUL);
            index++;
            buffer[index] = (unsigned char)((animalSleepStamp[i] >> 8) & 0xFFUL);
            index++;
            buffer[index] = (unsigned char)((animalSleepStamp[i] >> 16) & 0xFFUL);
            index++;
            buffer[index] = (unsigned char)((animalSleepStamp[i] >> 24) & 0xFFUL);
            index++;
        } else {
            buffer[index] = 0;
            index++;
            buffer[index] = 0;
            index++;
            buffer[index] = 0;
            index++;
            buffer[index] = 0;
            index++;
            buffer[index] = 0;
            index++;
        }
    }
}

void Farm_ImportState (const unsigned char *buffer) {
    unsigned char i;
    unsigned char index = 0;

    Farm_ClearCurrentData();

    for (i = 0; i < sizeof(farmName); i++) {
        farmName[i] = (char)buffer[index];
        index++;
    }
    farmName[FARM_MAX_NAME] = '\0';

    for (i = 0; i < FARM_NUM_SPECIES; i++) {
        Farm_SetGenerationTimeBySpecies(i, buffer[index]);
        index++;
    }

    totalAnimals = buffer[index];
    index++;
    if (totalAnimals > FARM_MAX_ANIMALS) {
        totalAnimals = FARM_MAX_ANIMALS;
    }

    for (i = 0; i < FARM_NUM_SPECIES; i++) {
        Farm_SetProductCountBySpecies(i, buffer[index]);
        index++;
    }

    for (i = 0; i < FARM_MAX_ANIMALS; i++) {
        animalInfo[i] = buffer[index];
        index++;
        if (Farm_GetAnimalSpecies(i) >= FARM_NUM_SPECIES) {
            Farm_SetAnimalSpecies(i, SPECIES_COW);
        }
        animalSleepStamp[i] = (unsigned long)buffer[index];
        index++;
        animalSleepStamp[i] |= ((unsigned long)buffer[index] << 8);
        index++;
        animalSleepStamp[i] |= ((unsigned long)buffer[index] << 16);
        index++;
        animalSleepStamp[i] |= ((unsigned long)buffer[index] << 24);
        index++;
        Farm_SetAnimalCritical(i, 0);
    }

    Farm_RecountAnimals();
    configured = 1;
    dirtyState = 0;
}

unsigned char Farm_IsDirty (void) {
    return dirtyState;
}

void Farm_ClearDirty (void) {
    dirtyState = 0;
}

static void resetFarmData (void) {
    unsigned char i;

    configured = 0;
    rebellion = 0;
    resetRequested = 0;
    dirtyState = 0;
    currentDateValid = 0;
    currentDay = 0;
    currentMonth = 0;
    currentHour = 0;
    currentMinute = 0;
    currentSecond = 0;
    farmName[0] = '\0';
    pendingName = 0;
    configRequested = 0;
    totalAnimals = 0;
    notificationHead = 0;
    notificationCount = 0;
    Farm_ResetSelectionState();

    for (i = 0; i < FARM_NUM_SPECIES; i++) {
        Farm_SetPendingTime(i, 0);
        Farm_SetGenerationTimeBySpecies(i, 0);
        Farm_SetAnimalCountBySpecies(i, 0);
        Farm_SetCriticalCountBySpecies(i, 0);
        Farm_SetProductCountBySpecies(i, 0);
        Farm_SetLastGenerationBySpecies(i, 0);
        Farm_SetLastProductBySpecies(i, 0);
    }

    for (i = 0; i < FARM_MAX_ANIMALS; i++) {
        animalInfo[i] = 0;
        animalSleepStamp[i] = 0;
    }
}

static void Farm_ClearCurrentData (void) {
    unsigned char i;

    totalAnimals = 0;
    notificationHead = 0;
    notificationCount = 0;
    rebellion = 0;
    dirtyState = 0;
    Farm_ResetSelectionState();

    for (i = 0; i < FARM_NUM_SPECIES; i++) {
        Farm_SetAnimalCountBySpecies(i, 0);
        Farm_SetCriticalCountBySpecies(i, 0);
        Farm_SetProductCountBySpecies(i, 0);
        Farm_SetLastGenerationBySpecies(i, 0);
        Farm_SetLastProductBySpecies(i, 0);
    }

    for (i = 0; i < FARM_MAX_ANIMALS; i++) {
        animalInfo[i] = 0;
        animalSleepStamp[i] = 0;
    }
}

static void Farm_RecountAnimals (void) {
    unsigned char i;

    for (i = 0; i < FARM_NUM_SPECIES; i++) {
        Farm_SetAnimalCountBySpecies(i, 0);
        Farm_SetCriticalCountBySpecies(i, 0);
    }

    for (i = 0; i < totalAnimals; i++) {
        Farm_SetAnimalCountBySpecies(Farm_GetAnimalSpecies(i), (unsigned char)(Farm_GetAnimalCountBySpecies(Farm_GetAnimalSpecies(i)) + 1));
        if (Farm_IsAnimalCritical(i) == 1) {
            Farm_SetCriticalCountBySpecies(Farm_GetAnimalSpecies(i), (unsigned char)(Farm_GetCriticalCountBySpecies(Farm_GetAnimalSpecies(i)) + 1));
        }
    }
}

static void Farm_MarkDirty (void) {
    dirtyState = 1;
}

static void Farm_PushNotification (unsigned char kind, unsigned char species, unsigned char number) {
    unsigned char tail;

    if (notificationCount >= FARM_NOTIFICATION_QUEUE_SIZE) {
        return;
    }

    tail = (unsigned char)(notificationHead + notificationCount);
    if (tail >= FARM_NOTIFICATION_QUEUE_SIZE) {
        tail = (unsigned char)(tail - FARM_NOTIFICATION_QUEUE_SIZE);
    }

    notificationMeta[tail] = (unsigned char)(((kind & 0x3F) << 2) | (species & 0x03));
    Farm_SetNotificationValue(tail, number);
    notificationCount++;
}

static unsigned char Farm_CreateAnimal (unsigned char species) {
    unsigned char missingAnimals = 0;
    unsigned char i;

    if (totalAnimals >= FARM_MAX_ANIMALS) {
        return 0;
    }

    for (i = 0; i < FARM_NUM_SPECIES; i++) {
        unsigned char count = Farm_GetAnimalCountBySpecies(i);
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

    animalInfo[totalAnimals] = 0;
    Farm_SetAnimalSpecies(totalAnimals, species);
    Farm_SetAnimalCritical(totalAnimals, 0);
    Farm_StoreSleepDate(totalAnimals);
    totalAnimals++;
    return 1;
}

static unsigned char Farm_GetAwakeCount (unsigned char species) {
    return (unsigned char)(Farm_GetAnimalCountBySpecies(species) - Farm_GetCriticalCountBySpecies(species));
}

static void Farm_TakeProduct (unsigned char species, unsigned char amount) {
    if (Farm_GetProductCountBySpecies(species) >= amount) {
        Farm_SetProductCountBySpecies(species, (unsigned char)(Farm_GetProductCountBySpecies(species) - amount));
    }
}

static void Farm_ProcessGeneration (unsigned char species, unsigned char now) {
    if (Farm_GetGenerationTimeBySpecies(species) == 0) {
        return;
    }

    if ((unsigned char)(now - Farm_GetLastGenerationBySpecies(species)) >= Farm_GetGenerationTimeBySpecies(species)) {
        if (Farm_CreateAnimal(species) == 1) {
            Farm_SetAnimalCountBySpecies(species, (unsigned char)(Farm_GetAnimalCountBySpecies(species) + 1));
            Farm_PushNotification(FARM_NOTIFICATION_ANIMAL, species, Farm_GetAnimalCountBySpecies(species));
            Farm_MarkDirty();
        }
        Farm_SetLastGenerationBySpecies(species, now);
    }
}

static void Farm_ProcessProducts (unsigned char species, unsigned char now) {
    unsigned char awakeCount;

    if (rebellion == 1) {
        return;
    }

    if ((unsigned char)(now - Farm_GetLastProductBySpecies(species)) < Farm_GetProductTimeBySpecies(species)) {
        return;
    }

    awakeCount = Farm_GetAwakeCount(species);
    if (awakeCount > 0) {
        if ((unsigned int)Farm_GetProductCountBySpecies(species) + awakeCount > 255) {
            Farm_SetProductCountBySpecies(species, 255);
        } else {
            Farm_SetProductCountBySpecies(species, (unsigned char)(Farm_GetProductCountBySpecies(species) + awakeCount));
        }
        Farm_PushNotification(FARM_NOTIFICATION_PRODUCT, species, Farm_GetProductCountBySpecies(species));
        Farm_MarkDirty();
    }
    Farm_SetLastProductBySpecies(species, now);
}

static void Farm_ProcessCriticalAnimal (unsigned char index) {
    unsigned char species;

    if (Farm_IsAnimalCritical(index) == 1) {
        return;
    }

    if (currentDateValid == 0) {
        return;
    }

    if (Farm_GetSleepElapsedSeconds(index) < (CRITICAL_TIME / 1000UL)) {
        return;
    }

    species = Farm_GetAnimalSpecies(index);
    Farm_SetAnimalCritical(index, 1);
    Farm_SetCriticalCountBySpecies(species, (unsigned char)(Farm_GetCriticalCountBySpecies(species) + 1));
    Farm_MarkDirty();
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
    static const unsigned char daysPerMonth[12] = {
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };
    unsigned char i;
    unsigned long totalDays = 0;

    if (month == 0) {
        month = 1;
    }
    if (day == 0) {
        day = 1;
    }

    for (i = 1; i < month && i <= 12; i++) {
        totalDays += daysPerMonth[i - 1];
    }
    totalDays += (unsigned long)(day - 1);

    return (((totalDays * 24UL) + hour) * 60UL + minute) * 60UL + second;
}

static unsigned long Farm_GetCurrentDateStamp (void) {
    return Farm_BuildDateSeconds(currentDay, currentMonth, currentHour, currentMinute, currentSecond);
}

static unsigned long Farm_GetSleepElapsedSeconds (unsigned char index) {
    unsigned long currentTotal;
    unsigned long sleepTotal;
    const unsigned long yearSeconds = 31536000UL;

    currentTotal = Farm_GetCurrentDateStamp();
    sleepTotal = animalSleepStamp[index];

    if (currentTotal >= sleepTotal) {
        return currentTotal - sleepTotal;
    }

    return (yearSeconds - sleepTotal) + currentTotal;
}

static void Farm_StoreSleepDate (unsigned char index) {
    animalSleepStamp[index] = Farm_GetCurrentDateStamp();
}

static unsigned char Farm_GetAnimalSpecies (unsigned char index) {
    return (unsigned char)(animalInfo[index] & ANIMAL_SPECIES_MASK);
}

static unsigned char Farm_IsAnimalCritical (unsigned char index) {
    if ((animalInfo[index] & ANIMAL_CRITICAL_MASK) != 0) {
        return 1;
    }
    return 0;
}

static void Farm_SetAnimalSpecies (unsigned char index, unsigned char species) {
    animalInfo[index] = (unsigned char)((animalInfo[index] & ANIMAL_CRITICAL_MASK) | (species & ANIMAL_SPECIES_MASK));
}

static void Farm_SetAnimalCritical (unsigned char index, unsigned char critical) {
    if (critical == 1) {
        animalInfo[index] |= ANIMAL_CRITICAL_MASK;
    } else {
        animalInfo[index] &= (unsigned char)(~ANIMAL_CRITICAL_MASK);
    }
}

static unsigned char Farm_GetPendingTime (unsigned char species) {
    switch (species) {
        case SPECIES_COW: return pendingTimeCow;
        case SPECIES_PIG: return pendingTimePig;
        case SPECIES_HORSE: return pendingTimeHorse;
        default: return pendingTimeChicken;
    }
}

static void Farm_SetPendingTime (unsigned char species, unsigned char value) {
    switch (species) {
        case SPECIES_COW: pendingTimeCow = value; break;
        case SPECIES_PIG: pendingTimePig = value; break;
        case SPECIES_HORSE: pendingTimeHorse = value; break;
        default: pendingTimeChicken = value; break;
    }
}

static unsigned char Farm_GetGenerationTimeBySpecies (unsigned char species) {
    switch (species) {
        case SPECIES_COW: return generationTimeCow;
        case SPECIES_PIG: return generationTimePig;
        case SPECIES_HORSE: return generationTimeHorse;
        default: return generationTimeChicken;
    }
}

static void Farm_SetGenerationTimeBySpecies (unsigned char species, unsigned char value) {
    switch (species) {
        case SPECIES_COW: generationTimeCow = value; break;
        case SPECIES_PIG: generationTimePig = value; break;
        case SPECIES_HORSE: generationTimeHorse = value; break;
        default: generationTimeChicken = value; break;
    }
}

static unsigned char Farm_GetAnimalCountBySpecies (unsigned char species) {
    switch (species) {
        case SPECIES_COW: return animalCountCow;
        case SPECIES_PIG: return animalCountPig;
        case SPECIES_HORSE: return animalCountHorse;
        default: return animalCountChicken;
    }
}

static void Farm_SetAnimalCountBySpecies (unsigned char species, unsigned char value) {
    switch (species) {
        case SPECIES_COW: animalCountCow = value; break;
        case SPECIES_PIG: animalCountPig = value; break;
        case SPECIES_HORSE: animalCountHorse = value; break;
        default: animalCountChicken = value; break;
    }
}

static unsigned char Farm_GetCriticalCountBySpecies (unsigned char species) {
    switch (species) {
        case SPECIES_COW: return criticalCountCow;
        case SPECIES_PIG: return criticalCountPig;
        case SPECIES_HORSE: return criticalCountHorse;
        default: return criticalCountChicken;
    }
}

static void Farm_SetCriticalCountBySpecies (unsigned char species, unsigned char value) {
    switch (species) {
        case SPECIES_COW: criticalCountCow = value; break;
        case SPECIES_PIG: criticalCountPig = value; break;
        case SPECIES_HORSE: criticalCountHorse = value; break;
        default: criticalCountChicken = value; break;
    }
}

static unsigned char Farm_GetProductCountBySpecies (unsigned char species) {
    switch (species) {
        case SPECIES_COW: return productCountCow;
        case SPECIES_PIG: return productCountPig;
        case SPECIES_HORSE: return productCountHorse;
        default: return productCountChicken;
    }
}

static void Farm_SetProductCountBySpecies (unsigned char species, unsigned char value) {
    switch (species) {
        case SPECIES_COW: productCountCow = value; break;
        case SPECIES_PIG: productCountPig = value; break;
        case SPECIES_HORSE: productCountHorse = value; break;
        default: productCountChicken = value; break;
    }
}

static unsigned char Farm_GetLastGenerationBySpecies (unsigned char species) {
    switch (species) {
        case SPECIES_COW: return lastGenerationCow;
        case SPECIES_PIG: return lastGenerationPig;
        case SPECIES_HORSE: return lastGenerationHorse;
        default: return lastGenerationChicken;
    }
}

static void Farm_SetLastGenerationBySpecies (unsigned char species, unsigned char value) {
    switch (species) {
        case SPECIES_COW: lastGenerationCow = value; break;
        case SPECIES_PIG: lastGenerationPig = value; break;
        case SPECIES_HORSE: lastGenerationHorse = value; break;
        default: lastGenerationChicken = value; break;
    }
}

static unsigned char Farm_GetLastProductBySpecies (unsigned char species) {
    switch (species) {
        case SPECIES_COW: return lastProductCow;
        case SPECIES_PIG: return lastProductPig;
        case SPECIES_HORSE: return lastProductHorse;
        default: return lastProductChicken;
    }
}

static void Farm_SetLastProductBySpecies (unsigned char species, unsigned char value) {
    switch (species) {
        case SPECIES_COW: lastProductCow = value; break;
        case SPECIES_PIG: lastProductPig = value; break;
        case SPECIES_HORSE: lastProductHorse = value; break;
        default: lastProductChicken = value; break;
    }
}

static unsigned char Farm_GetProductTimeBySpecies (unsigned char species) {
    switch (species) {
        case SPECIES_COW: return 47;
        case SPECIES_PIG: return 31;
        case SPECIES_HORSE: return 23;
        default: return 13;
    }
}

static void Farm_SetNotificationValue (unsigned char index, unsigned char value) {
    if (index < 4) {
        notificationValueA[index] = value;
    } else {
        notificationValueB[index - 4] = value;
    }
}

static unsigned char Farm_GetNotificationValue (unsigned char index) {
    if (index < 4) {
        return notificationValueA[index];
    }
    return notificationValueB[index - 4];
}
