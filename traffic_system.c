/*
 * ============================================================
 *        TRAFFIC SIGNAL MANAGEMENT SYSTEM — MERGED
 *        Combines animated cycle simulation + full management
 * ============================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
  #include <windows.h>
  #define SLEEP(ms) Sleep(ms)
  #define CLEAR     "cls"
#else
  #include <unistd.h>
  #define SLEEP(ms) usleep((ms) * 1000)
  #define CLEAR     "clear"
#endif

/* ─────────────────────── CONSTANTS ─────────────────────── */
#define MAX_SIGNALS     8
#define MAX_LOG         50
#define NUM_DIRS        4
#define MIN_GREEN       5
#define MAX_GREEN       60
#define YELLOW_DUR      3
#define MAX_VEHICLES    500
#define EMERGENCY_TIME  60

/* ─────────────────────── ENUMS ─────────────────────── */
typedef enum { RED, YELLOW, GREEN, EMERGENCY } SignalState;
typedef enum { LOW, MEDIUM, HIGH, CRITICAL }   TrafficDensity;

/* ─────────────────────── STRUCTURES ─────────────────────── */
typedef struct {
    int            id;
    char           location[50];
    char           direction[6];   /* NORTH / SOUTH / EAST / WEST */
    SignalState    state;
    int            timeRemaining;
    int            vehicleCount;
    TrafficDensity density;
    int            isEmergency;
    int            totalVehiclesPassed;
    int            cycleCount;
    int            greenDuration;  /* adaptive green time (seconds) */
} TrafficSignal;

typedef struct {
    char message[100];
    char timestamp[25];
    int  signalId;
} LogEntry;

typedef struct {
    TrafficSignal signals[MAX_SIGNALS];
    LogEntry      logs[MAX_LOG];
    int           signalCount;
    int           logCount;
} TrafficSystem;

/* ─────────────────────── GLOBAL ─────────────────────── */
TrafficSystem sys;

/* ══════════════════════ UTILITY ══════════════════════ */

void getCurrentTime(char *buf) {
    time_t t = time(NULL);
    strftime(buf, 25, "%Y-%m-%d %H:%M:%S", localtime(&t));
}

void addLog(int id, const char *msg) {
    if (sys.logCount >= MAX_LOG) {
        for (int i = 0; i < MAX_LOG - 1; i++)
            sys.logs[i] = sys.logs[i + 1];
        sys.logCount = MAX_LOG - 1;
    }
    LogEntry *e = &sys.logs[sys.logCount++];
    e->signalId = id;
    strncpy(e->message, msg, 99);
    getCurrentTime(e->timestamp);
}

void divider(char ch, int len) {
    for (int i = 0; i < len; i++) putchar(ch);
    putchar('\n');
}

const char *stateStr(SignalState s) {
    switch (s) {
        case GREEN:     return "GREEN    ";
        case YELLOW:    return "YELLOW   ";
        case RED:       return "RED      ";
        case EMERGENCY: return "EMERGENCY";
        default:        return "UNKNOWN  ";
    }
}

const char *densityStr(TrafficDensity d) {
    switch (d) {
        case LOW:      return "LOW     ";
        case MEDIUM:   return "MEDIUM  ";
        case HIGH:     return "HIGH    ";
        case CRITICAL: return "CRITICAL";
        default:       return "UNKNOWN ";
    }
}

TrafficDensity calcDensity(int count) {
    if (count < 10)  return LOW;
    if (count < 25)  return MEDIUM;
    if (count < 40)  return HIGH;
    return CRITICAL;
}

/* Adaptive green time based on density */
int adaptiveGreen(TrafficDensity d) {
    switch (d) {
        case LOW:      return 15;
        case MEDIUM:   return 30;
        case HIGH:     return 45;
        case CRITICAL: return MAX_GREEN;
        default:       return 30;
    }
}

/* Green duration from raw vehicle count (for simulation) */
int calcGreenFromCount(int count) {
    if (count == 0) return MIN_GREEN;
    int dur = (int)(count * 0.15);
    if (dur < MIN_GREEN) dur = MIN_GREEN;
    if (dur > MAX_GREEN) dur = MAX_GREEN;
    return dur;
}

/* ══════════════════════ SIGNAL MANAGEMENT ══════════════════════ */

void initSystem(void) {
    memset(&sys, 0, sizeof(TrafficSystem));
    addLog(-1, "System initialized.");
    printf("\n  [SYSTEM] Traffic Signal Management System Initialized.\n");
}

int addSignal(const char *location, const char *dir) {
    if (sys.signalCount >= MAX_SIGNALS) {
        printf("  [ERROR] Max signal limit reached.\n"); return -1;
    }
    TrafficSignal *s = &sys.signals[sys.signalCount];
    s->id               = sys.signalCount + 1;
    strncpy(s->location,  location, 49);
    strncpy(s->direction, dir,      5);
    s->state            = RED;
    s->timeRemaining    = 30;
    s->vehicleCount     = rand() % (MAX_VEHICLES + 1);
    s->density          = calcDensity(s->vehicleCount);
    s->greenDuration    = adaptiveGreen(s->density);
    s->isEmergency      = 0;
    s->totalVehiclesPassed = 0;
    s->cycleCount       = 0;
    sys.signalCount++;

    char msg[100];
    snprintf(msg, 100, "Signal #%d added at '%s' [%s]", s->id, location, dir);
    addLog(s->id, msg);
    printf("  [ADD] Signal #%d | %s | %s | Vehicles: %d | Green: %ds\n",
           s->id, location, dir, s->vehicleCount, s->greenDuration);
    return s->id;
}

void updateVehicleCount(int id, int count) {
    if (id < 1 || id > sys.signalCount) { printf("  [ERROR] Invalid ID.\n"); return; }
    TrafficSignal *s = &sys.signals[id - 1];
    s->vehicleCount  = count;
    s->density       = calcDensity(count);
    s->greenDuration = adaptiveGreen(s->density);
    char msg[100];
    snprintf(msg, 100, "Vehicles -> %d | Density: %s | Green: %ds",
             count, densityStr(s->density), s->greenDuration);
    addLog(id, msg);
    printf("  [UPDATE] Signal #%d -> Vehicles: %d | %s | Green: %ds\n",
           id, count, densityStr(s->density), s->greenDuration);
}

void triggerEmergency(int id) {
    if (id < 1 || id > sys.signalCount) { printf("  [ERROR] Invalid ID.\n"); return; }
    TrafficSignal *s = &sys.signals[id - 1];
    s->isEmergency   = 1;
    s->state         = EMERGENCY;
    s->timeRemaining = EMERGENCY_TIME;
    addLog(id, "EMERGENCY MODE ACTIVATED");
    printf("  [!!!] EMERGENCY on Signal #%d at '%s'!\n", id, s->location);
}

void clearEmergency(int id) {
    if (id < 1 || id > sys.signalCount) return;
    TrafficSignal *s = &sys.signals[id - 1];
    s->isEmergency   = 0;
    s->state         = RED;
    s->timeRemaining = 30;
    addLog(id, "Emergency cleared.");
    printf("  [OK] Emergency cleared on Signal #%d.\n", id);
}

void tickSignal(TrafficSignal *s) {
    if (s->isEmergency) { s->state = EMERGENCY; s->timeRemaining = EMERGENCY_TIME; return; }
    if (--s->timeRemaining <= 0) {
        switch (s->state) {
            case GREEN:
                s->state = YELLOW; s->timeRemaining = YELLOW_DUR;
                s->totalVehiclesPassed += s->vehicleCount; break;
            case YELLOW:
                s->state = RED;   s->timeRemaining = 30; s->cycleCount++; break;
            case RED:
                s->state = GREEN; s->timeRemaining = s->greenDuration; break;
            default:
                s->state = RED;   s->timeRemaining = 30; break;
        }
        char msg[100];
        snprintf(msg, 100, "State -> %s | Timer: %ds", stateStr(s->state), s->timeRemaining);
        addLog(s->id, msg);
    }
}

/* ══════════════════════ ANIMATED CYCLE SIMULATION ══════════════════════ */

void printSignalIcon(SignalState st) {
    switch (st) {
        case GREEN:     printf("[GRN]"); break;
        case YELLOW:    printf("[YLW]"); break;
        case RED:       printf("[RED]"); break;
        case EMERGENCY: printf("[!!!]"); break;
    }
}

void printBar(int count, int max) {
    int w = 28, fill = (max > 0) ? (count * w / max) : 0;
    if (fill > w) fill = w;
    printf("[");
    for (int i = 0; i < w; i++) putchar(i < fill ? '#' : '.');
    printf("]");
}

void animateCycle(int minutes, int delayMs) {
    if (sys.signalCount == 0) { printf("  [ERROR] No signals to simulate.\n"); return; }

    const char *arrows[] = {"[^]","[>]","[v]","[<]"};

    for (int min = 1; min <= minutes; min++) {
        /* Randomise vehicle counts each minute */
        for (int i = 0; i < sys.signalCount; i++) {
            TrafficSignal *s = &sys.signals[i];
            s->vehicleCount  = rand() % (MAX_VEHICLES + 1);
            s->density       = calcDensity(s->vehicleCount);
            s->greenDuration = calcGreenFromCount(s->vehicleCount);
            s->state         = RED;
        }

        /* Compute total cycle length across all directions */
        int cycle = 0;
        for (int i = 0; i < sys.signalCount; i++)
            cycle += sys.signals[i].greenDuration + YELLOW_DUR;

        for (int sec = 0; sec < cycle; sec++) {
            /* Determine which direction is active this second */
            int t = sec, active = 0;
            for (int i = 0; i < sys.signalCount; i++) {
                int phase = sys.signals[i].greenDuration + YELLOW_DUR;
                if (t < phase) { active = i; break; }
                t -= phase;
            }
            /* Assign states */
            int t2 = sec;
            for (int i = 0; i < sys.signalCount; i++) {
                int phase = sys.signals[i].greenDuration + YELLOW_DUR;
                if (i == active) {
                    sys.signals[i].state = (t2 < sys.signals[i].greenDuration) ? GREEN : YELLOW;
                } else {
                    sys.signals[i].state = RED;
                }
                t2 -= phase;
            }

            if (delayMs == 0) continue;   /* instant mode: skip frame rendering */

            system(CLEAR);
            divider('=', 62);
            printf("||%*s TRAFFIC CYCLE SIMULATION %*s||\n", 9, "", 8, "");
            divider('=', 62);
            printf("  Minute: %d / %d     Cycle Second: %d / %d\n",
                   min, minutes, sec + 1, cycle);
            divider('-', 62);

            for (int i = 0; i < sys.signalCount && i < NUM_DIRS; i++) {
                TrafficSignal *s = &sys.signals[i];
                int isActive = (s->state == GREEN || s->state == YELLOW);
                printf("\n  %s  %-5s @ %-22s%s\n",
                       (i < 4 ? arrows[i] : "[*]"),
                       s->direction, s->location,
                       isActive ? "  <<< ACTIVE >>>" : "");
                divider('-', 42);
                printf("  Vehicles  : %-4d  ", s->vehicleCount);
                printBar(s->vehicleCount, MAX_VEHICLES);
                printf("\n  Green Time: %ds | Signal: ", s->greenDuration);
                printSignalIcon(s->state);
                putchar('\n');
            }

            divider('=', 62);
            printf("  [Ctrl+C to abort]\n");
            SLEEP(delayMs);
        }
        printf("  [Minute %d complete]\n", min);
    }
    printf("  [DONE] Animated simulation complete.\n");
}

/* ══════════════════════ DISPLAY / REPORTS ══════════════════════ */

void displayAllSignals(void) {
    printf("\n");
    divider('=', 68);
    printf("  %-4s %-22s %-6s %-12s %-6s %-9s %-8s\n",
           "ID", "LOCATION", "DIR", "STATE", "TIMER", "VEHICLES", "DENSITY");
    divider('-', 68);
    for (int i = 0; i < sys.signalCount; i++) {
        TrafficSignal *s = &sys.signals[i];
        printf("  #%-3d %-22s %-6s %-12s %-6d %-9d %-8s%s\n",
               s->id, s->location, s->direction, stateStr(s->state),
               s->timeRemaining, s->vehicleCount, densityStr(s->density),
               s->isEmergency ? " [!!!]" : "");
    }
    divider('=', 68);
    printf("  Total Signals: %d\n\n", sys.signalCount);
}

void displaySignalDetail(int id) {
    if (id < 1 || id > sys.signalCount) { printf("  [ERROR] Invalid ID.\n"); return; }
    TrafficSignal *s = &sys.signals[id - 1];
    divider('=', 50);
    printf("  SIGNAL #%d  —  %s\n", s->id, s->location);
    divider('-', 50);
    printf("  Direction       : %s\n",  s->direction);
    printf("  State           : %s\n",  stateStr(s->state));
    printf("  Timer Left      : %d s\n", s->timeRemaining);
    printf("  Green Duration  : %d s\n", s->greenDuration);
    printf("  Vehicles        : %d\n",  s->vehicleCount);
    printf("  Density         : %s\n",  densityStr(s->density));
    printf("  Emergency       : %s\n",  s->isEmergency ? "YES !!!" : "No");
    printf("  Cycles Done     : %d\n",  s->cycleCount);
    printf("  Total Passed    : %d\n",  s->totalVehiclesPassed);
    divider('=', 50);
}

void displayStats(void) {
    int totalV = 0, totalC = 0, emg = 0;
    for (int i = 0; i < sys.signalCount; i++) {
        totalV += sys.signals[i].totalVehiclesPassed;
        totalC += sys.signals[i].cycleCount;
        if (sys.signals[i].isEmergency) emg++;
    }
    printf("\n");
    divider('=', 50);
    printf("  SYSTEM STATISTICS\n");
    divider('-', 50);
    printf("  Total Signals        : %d\n", sys.signalCount);
    printf("  Total Vehicles Passed: %d\n", totalV);
    printf("  Total Signal Cycles  : %d\n", totalC);
    printf("  Active Emergencies   : %d\n", emg);
    printf("  Log Entries          : %d\n", sys.logCount);
    divider('=', 50);
}

void displayLogs(int last) {
    divider('=', 70);
    printf("  SYSTEM LOGS (last %d entries)\n", last);
    divider('-', 70);
    int start = sys.logCount - last;
    if (start < 0) start = 0;
    for (int i = start; i < sys.logCount; i++) {
        LogEntry *e = &sys.logs[i];
        printf("  [%s] SIG#%-2d  %s\n", e->timestamp, e->signalId, e->message);
    }
    divider('=', 70);
}

/* ══════════════════════ MENU & MAIN ══════════════════════ */

void printMenu(void) {
    printf("\n  ╔══════════════════════════════════════╗\n");
    printf("  ║   TRAFFIC SIGNAL MANAGEMENT SYSTEM  ║\n");
    printf("  ╠══════════════════════════════════════╣\n");
    printf("  ║  1. Add Signal                       ║\n");
    printf("  ║  2. View All Signals                 ║\n");
    printf("  ║  3. View Signal Detail               ║\n");
    printf("  ║  4. Update Vehicle Count             ║\n");
    printf("  ║  5. Trigger Emergency                ║\n");
    printf("  ║  6. Clear Emergency                  ║\n");
    printf("  ║  7. Run Tick Simulation              ║\n");
    printf("  ║  8. Run Animated Cycle Simulation    ║\n");
    printf("  ║  9. View System Logs                 ║\n");
    printf("  ║  10. View Statistics                 ║\n");
    printf("  ║  0. Exit                             ║\n");
    printf("  ╚══════════════════════════════════════╝\n");
    printf("  Enter choice: ");
}

void loadDemo(void) {
    printf("\n  [DEMO] Loading demo scenario...\n");
    addSignal("Main St & 1st Ave",   "NORTH");
    addSignal("Broadway & 5th Ave",  "SOUTH");
    addSignal("Park Rd & Oak Blvd",  "EAST");
    addSignal("Highway 10 Junction", "WEST");
    updateVehicleCount(2, 35);
    updateVehicleCount(4, 48);
    displayAllSignals();
}

int main(void) {
    srand((unsigned)time(NULL));
    initSystem();

    int choice;
    printf("\n  Run DEMO first? (1=Yes / 0=No): ");
    scanf("%d", &choice);
    if (choice == 1) loadDemo();

    while (1) {
        printMenu();
        scanf("%d", &choice);

        int id, count, seconds, minutes, speed;
        char location[50], dir[6];

        switch (choice) {
            case 1:
                printf("  Location name : "); scanf(" %49[^\n]", location);
                printf("  Direction (NORTH/SOUTH/EAST/WEST): "); scanf(" %5s", dir);
                addSignal(location, dir);
                break;
            case 2:
                displayAllSignals(); break;
            case 3:
                printf("  Signal ID: "); scanf("%d", &id);
                displaySignalDetail(id); break;
            case 4:
                printf("  Signal ID: "); scanf("%d", &id);
                printf("  Vehicle count: "); scanf("%d", &count);
                updateVehicleCount(id, count); break;
            case 5:
                printf("  Signal ID for emergency: "); scanf("%d", &id);
                triggerEmergency(id); break;
            case 6:
                printf("  Signal ID to clear: "); scanf("%d", &id);
                clearEmergency(id); break;
            case 7:
                printf("  Simulation duration (seconds): "); scanf("%d", &seconds);
                printf("\n  [TICK SIM] Running %d seconds...\n", seconds);
                for (int t = 1; t <= seconds; t++) {
                    for (int i = 0; i < sys.signalCount; i++)
                        tickSignal(&sys.signals[i]);
                    if (t % 10 == 0) {
                        printf("  [T=%3ds]", t);
                        for (int i = 0; i < sys.signalCount; i++)
                            printf(" #%d:%s(%ds)", sys.signals[i].id,
                                   stateStr(sys.signals[i].state),
                                   sys.signals[i].timeRemaining);
                        putchar('\n');
                    }
                }
                printf("  [DONE]\n"); break;
            case 8:
                printf("  Minutes to animate (1-10): "); scanf("%d", &minutes);
                if (minutes < 1) minutes = 1; if (minutes > 10) minutes = 10;
                printf("  Speed (1=real-time, 2=fast, 3=instant): "); scanf("%d", &speed);
                animateCycle(minutes, speed == 1 ? 1000 : speed == 2 ? 200 : 0);
                break;
            case 9:
                printf("  Recent logs to show: "); scanf("%d", &count);
                displayLogs(count); break;
            case 10:
                displayStats(); break;
            case 0:
                printf("\n  [EXIT] System shutdown. Goodbye!\n\n");
                return 0;
            default:
                printf("  [ERROR] Invalid choice.\n");
        }
    }
}
/* ─────────────────── END OF FILE ─────────────────── */
