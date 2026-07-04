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

static const unsigned long productTime[FARM_NUM_SPECIES] = {
    47000UL, 31000UL, 23000UL, 13000UL
};

static unsigned char timerHandle;
static unsigned char configured;
static unsigned char rebellion;
static unsigned char resetRequested;
static unsigned char timeReady;
static unsigned char dirtyState;
static unsigned char currentDateValid;
static unsigned char currentDay;
static unsigned char currentMonth;
static unsigned char currentHour;
static unsigned char currentMinute;
static unsigned char currentSecond;

static char farmName[FARM_MAX_NAME + 1];
static const char *pendingName;
static unsigned char pendingTime[FARM_NUM_SPECIES];
static unsigned char configRequested;

static unsigned char generationTime[FARM_NUM_SPECIES];
static unsigned char animalCountBySpecies[FARM_NUM_SPECIES];
static unsigned char criticalCountBySpecies[FARM_NUM_SPECIES];
static unsigned char productCountBySpecies[FARM_NUM_SPECIES];

static unsigned char totalAnimals;
static unsigned char nextAnimalId;

static unsigned char selectedAnimalSpecies;
static unsigned char selectedAnimalNumber;
static signed char selectedAnimalIndex;
static unsigned char searchRequested;
static unsigned char searchFinished;
static unsigned char searchFound;
static unsigned char restRequestPending;
static unsigned char restFinished;
static unsigned char restSuccess;

static unsigned long lastGeneration[FARM_NUM_SPECIES];
static unsigned long lastProduct[FARM_NUM_SPECIES];

static unsigned char animalId[FARM_MAX_ANIMALS];
static unsigned char animalInfo[FARM_MAX_ANIMALS];
static unsigned long animalSleepStamp[FARM_MAX_ANIMALS];
static FarmNotification notificationQueue[FARM_NOTIFICATION_QUEUE_SIZE];
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
static void Farm_ProcessGeneration (unsigned char species, unsigned long now);
static void Farm_ProcessProducts (unsigned char species, unsigned long now);
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
    static unsigned long now = 0;

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
            generationTime[SPECIES_COW] = pendingTime[SPECIES_COW];
            generationTime[SPECIES_PIG] = pendingTime[SPECIES_PIG];
            generationTime[SPECIES_HORSE] = pendingTime[SPECIES_HORSE];
            generationTime[SPECIES_CHICKEN] = pendingTime[SPECIES_CHICKEN];
            configured = 1;
            configRequested = 0;
            Farm_MarkDirty();
            TI_ResetTics(timerHandle);
            state = 3;
            break;

        case 3:
            if (configured == 0 || timeReady == 0 || currentDateValid == 0) {
                break;
            }
            speciesIndex = 0;
            state = 4;
            break;

        case 4:
            now = TI_GetTics(timerHandle);
            if (speciesIndex < FARM_NUM_SPECIES) {
                Farm_ProcessGeneration(speciesIndex, now);
                speciesIndex++;
            } else {
                speciesIndex = 0;
                state = 5;
            }
            break;

        case 5:
            now = TI_GetTics(timerHandle);
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
    pendingTime[SPECIES_COW] = cow;
    pendingTime[SPECIES_HORSE] = horse;
    pendingTime[SPECIES_PIG] = pig;
    pendingTime[SPECIES_CHICKEN] = chicken;
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
        if (criticalCountBySpecies[species] > 0) {
            criticalCountBySpecies[species]--;
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
    return generationTime[SPECIES_COW];
}

unsigned char Farm_GetPig (void) {
    return generationTime[SPECIES_PIG];
}

unsigned char Farm_GetHorse (void) {
    return generationTime[SPECIES_HORSE];
}

unsigned char Farm_GetChicken (void) {
    return generationTime[SPECIES_CHICKEN];
}

unsigned char Farm_GetNumCow (void) {
    return animalCountBySpecies[SPECIES_COW];
}

unsigned char Farm_GetNumPig (void) {
    return animalCountBySpecies[SPECIES_PIG];
}

unsigned char Farm_GetNumHorse (void) {
    return animalCountBySpecies[SPECIES_HORSE];
}

unsigned char Farm_GetNumChicken (void) {
    return animalCountBySpecies[SPECIES_CHICKEN];
}

unsigned char Farm_GetMilk (void) {
    return productCountBySpecies[SPECIES_COW];
}

unsigned char Farm_GetHam (void) {
    return productCountBySpecies[SPECIES_PIG];
}

unsigned char Farm_GetBrush (void) {
    return productCountBySpecies[SPECIES_HORSE];
}

unsigned char Farm_GetEggs (void) {
    return productCountBySpecies[SPECIES_CHICKEN];
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
    return productCountBySpecies[species];
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

    *notification = notificationQueue[notificationHead];
    notificationHead++;
    if (notificationHead >= FARM_NOTIFICATION_QUEUE_SIZE) {
        notificationHead = 0;
    }
    notificationCount--;
    return 1;
}

void Farm_SetTimeReady (unsigned char ready) {
    timeReady = ready;
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
        buffer[index] = generationTime[i];
        index++;
    }

    buffer[index] = totalAnimals;
    index++;
    buffer[index] = nextAnimalId;
    index++;

    for (i = 0; i < FARM_NUM_SPECIES; i++) {
        buffer[index] = productCountBySpecies[i];
        index++;
    }

    for (i = 0; i < FARM_MAX_ANIMALS; i++) {
        if (i < totalAnimals) {
            buffer[index] = animalId[i];
            index++;
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
        generationTime[i] = buffer[index];
        index++;
    }

    totalAnimals = buffer[index];
    index++;
    if (totalAnimals > FARM_MAX_ANIMALS) {
        totalAnimals = FARM_MAX_ANIMALS;
    }

    nextAnimalId = buffer[index];
    index++;
    if (nextAnimalId == 0) {
        nextAnimalId = 1;
    }

    for (i = 0; i < FARM_NUM_SPECIES; i++) {
        productCountBySpecies[i] = buffer[index];
        index++;
    }

    for (i = 0; i < FARM_MAX_ANIMALS; i++) {
        animalId[i] = buffer[index];
        index++;
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
    timeReady = 0;
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
    nextAnimalId = 1;
    notificationHead = 0;
    notificationCount = 0;
    Farm_ResetSelectionState();

    for (i = 0; i < FARM_NUM_SPECIES; i++) {
        pendingTime[i] = 0;
        generationTime[i] = 0;
        animalCountBySpecies[i] = 0;
        criticalCountBySpecies[i] = 0;
        productCountBySpecies[i] = 0;
        lastGeneration[i] = 0;
        lastProduct[i] = 0;
    }

    for (i = 0; i < FARM_MAX_ANIMALS; i++) {
        animalId[i] = 0;
        animalInfo[i] = 0;
        animalSleepStamp[i] = 0;
    }
}

static void Farm_ClearCurrentData (void) {
    unsigned char i;

    totalAnimals = 0;
    nextAnimalId = 1;
    notificationHead = 0;
    notificationCount = 0;
    rebellion = 0;
    dirtyState = 0;
    Farm_ResetSelectionState();

    for (i = 0; i < FARM_NUM_SPECIES; i++) {
        animalCountBySpecies[i] = 0;
        criticalCountBySpecies[i] = 0;
        productCountBySpecies[i] = 0;
        lastGeneration[i] = 0;
        lastProduct[i] = 0;
    }

    for (i = 0; i < FARM_MAX_ANIMALS; i++) {
        animalId[i] = 0;
        animalInfo[i] = 0;
        animalSleepStamp[i] = 0;
    }
}

static void Farm_RecountAnimals (void) {
    unsigned char i;

    for (i = 0; i < FARM_NUM_SPECIES; i++) {
        animalCountBySpecies[i] = 0;
        criticalCountBySpecies[i] = 0;
    }

    for (i = 0; i < totalAnimals; i++) {
        animalCountBySpecies[Farm_GetAnimalSpecies(i)]++;
        if (Farm_IsAnimalCritical(i) == 1) {
            criticalCountBySpecies[Farm_GetAnimalSpecies(i)]++;
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

    notificationQueue[tail].kind = kind;
    notificationQueue[tail].species = species;
    notificationQueue[tail].number = number;
    notificationCount++;
}

static unsigned char Farm_CreateAnimal (unsigned char species) {
    unsigned char nextCount[FARM_NUM_SPECIES];
    unsigned char missingAnimals = 0;
    unsigned char i;

    if (totalAnimals >= FARM_MAX_ANIMALS) {
        return 0;
    }

    for (i = 0; i < FARM_NUM_SPECIES; i++) {
        nextCount[i] = animalCountBySpecies[i];
    }
    nextCount[species]++;

    for (i = 0; i < FARM_NUM_SPECIES; i++) {
        if (nextCount[i] < FARM_MIN_ANIMALS_PER_SPECIES) {
            missingAnimals += (unsigned char)(FARM_MIN_ANIMALS_PER_SPECIES - nextCount[i]);
        }
    }

    if ((unsigned char)(totalAnimals + 1 + missingAnimals) > FARM_MAX_ANIMALS) {
        return 0;
    }

    animalId[totalAnimals] = nextAnimalId;
    animalInfo[totalAnimals] = 0;
    Farm_SetAnimalSpecies(totalAnimals, species);
    Farm_SetAnimalCritical(totalAnimals, 0);
    Farm_StoreSleepDate(totalAnimals);
    nextAnimalId++;
    totalAnimals++;
    return 1;
}

static unsigned char Farm_GetAwakeCount (unsigned char species) {
    return (unsigned char)(animalCountBySpecies[species] - criticalCountBySpecies[species]);
}

static void Farm_TakeProduct (unsigned char species, unsigned char amount) {
    if (productCountBySpecies[species] >= amount) {
        productCountBySpecies[species] -= amount;
    }
}

static void Farm_ProcessGeneration (unsigned char species, unsigned long now) {
    if (generationTime[species] == 0) {
        return;
    }

    if ((now - lastGeneration[species]) >= ((unsigned long)generationTime[species] * 1000UL)) {
        if (Farm_CreateAnimal(species) == 1) {
            animalCountBySpecies[species]++;
            Farm_PushNotification(FARM_NOTIFICATION_ANIMAL, species, animalCountBySpecies[species]);
            Farm_MarkDirty();
        }
        lastGeneration[species] = now;
    }
}

static void Farm_ProcessProducts (unsigned char species, unsigned long now) {
    unsigned char awakeCount;

    if (rebellion == 1) {
        return;
    }

    if ((now - lastProduct[species]) < productTime[species]) {
        return;
    }

    awakeCount = Farm_GetAwakeCount(species);
    if (awakeCount > 0) {
        if ((unsigned int)productCountBySpecies[species] + awakeCount > 255) {
            productCountBySpecies[species] = 255;
        } else {
            productCountBySpecies[species] += awakeCount;
        }
        Farm_PushNotification(FARM_NOTIFICATION_PRODUCT, species, productCountBySpecies[species]);
        Farm_MarkDirty();
    }
    lastProduct[species] = now;
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
    criticalCountBySpecies[species]++;
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
