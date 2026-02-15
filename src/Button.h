#ifndef Button_H
#define Button_H



/*#define BUTTON_PIN 0
#define BUTTON_PIN 1
#define BUTTON_PIN 3*/


class Button
{
public:
    bool lastState;
    int pin;
    Button(int pin, unsigned long debounceDelay = 50);
    bool activelow;

private:
    Button(int p, bool activeLow = true);
    bool isPressed();
    void begin();
   
};

#endif

















