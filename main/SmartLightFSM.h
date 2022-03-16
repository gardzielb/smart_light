//
// Created by bartosz on 3/3/22.
//

#ifndef SMART_LIGHT_SMARTLIGHTFSM_H
#define SMART_LIGHT_SMARTLIGHTFSM_H


class SmartLightState;

class SmartLightFSM {
public:
	void setState(SmartLightState* state);

	void loop();

	~SmartLightFSM();

private:
	SmartLightState* m_state = nullptr;
};


class SmartLightState {
public:
	virtual void begin() = 0;

	virtual void loop() = 0;

	inline virtual ~SmartLightState() {}

protected:
	inline SmartLightState(SmartLightFSM* fsm)
			: m_fsm(fsm) {}

	SmartLightFSM* const m_fsm;
};


#endif //SMART_LIGHT_SMARTLIGHTFSM_H
