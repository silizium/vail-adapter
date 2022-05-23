#pragma once

#define MAX_KEYER_QUEUE 5

typedef enum {
    PADDLE_DIT = 0,
    PADDLE_DAH,
    PADDLE_STRAIGHT,
} Paddle;

class Transmitter {
public:
    virtual void BeginTx();
    virtual void EndTx();
};

class Keyer {
public:
    virtual void Reset();
    virtual void SetDitDuration(int d);
    virtual void Release();
    virtual bool TxClosed();
    virtual bool TxClosed(int relay);
    virtual void Tx(int relay, bool closed);
    virtual void Key(Paddle key, bool pressed);
    virtual void Tick(unsigned int millis);
};

Keyer *GetKeyerByNumber(int n, Transmitter *output);
