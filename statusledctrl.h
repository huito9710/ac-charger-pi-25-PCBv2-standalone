#ifndef STATUSLEDCTRL_H
#define STATUSLEDCTRL_H


class StatusLedCtrl
{
public:
    // There are 4 states as defined in the STM32 software (LED.c m_WindowMsg)
    enum class State : int {
        ChargerBoard = 0, // Charger-Board decide
        white,
        green,
        yellow,
        flashinggreen,
        flashingred,
        blue,
        purple,
        RFIDError
    };
public:
    static StatusLedCtrl &instance() { static StatusLedCtrl ledCtrl; return ledCtrl; }
    void clear() { st = State::ChargerBoard; }
    State getState() { return st; }
    void setState(State newSt) { st = newSt; }

private:
    StatusLedCtrl() : st(State::ChargerBoard) {};
    State st;
};

#endif // STATUSLEDCTRL_H
