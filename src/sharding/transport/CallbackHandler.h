#ifndef __CALLBACK_HANDLER_H__
#define __CALLBACK_HANDLER_H__

class CallBackHandler {
public:
	virtual void notify(Message *msg) = 0;
	virtual ~CallBackHandler() {}
};


#endif /* __CALLBACK_H__ */
