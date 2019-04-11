#include "Framework.h"
#include "Input.h"

std::function<LRESULT(HWND, const uint&, const WPARAM&, const LPARAM&)> Input::MouseProc = nullptr;

Input::Input(const std::shared_ptr<Context>& context)
    : ISubsystem(context)
    , mousePosition(0)
    , wheelStatus(0)
    , wheelOldStatus(0)
    , wheelMoveValue(0)
{
    MouseProc = std::bind // 함수호출을 미리 정의해놓음
    (
        &Input::MsgProc,
        this,
        std::placeholders::_1,
        std::placeholders::_2,
        std::placeholders::_3,
        std::placeholders::_4   // 메시지가 아직 없으므로 공간을 미리 받아서 가지고있음.
    );

    EventSystem::Get().Subscribe(EventType::Event_Update, EVENT_HANDLER(Update));
}

// 헤더파일에 써놓음
LRESULT Input::MsgProc(HWND handle, const uint & message, const WPARAM & wParam, const LPARAM & lParam)
{
    if (message == WM_LBUTTONDOWN || message == WM_MOUSEMOVE)
    {
        mousePosition.x = static_cast<float>(LOWORD(lParam));
        mousePosition.y = static_cast<float>(HIWORD(lParam));
    }

    if (message == WM_MOUSEWHEEL)
    {
        short tWheelValue = static_cast<short>(HIWORD(wParam));

        wheelOldStatus.z = wheelStatus.z;
        wheelStatus.z += static_cast<float>(tWheelValue);
    }

    return TRUE;
}

const bool Input::Initialize()
{
	// 배열 0으로 초기화
    ZeroMemory(keyState, sizeof(keyState));
    ZeroMemory(keyOldState, sizeof(keyOldState));
    ZeroMemory(keyMap, sizeof(keyMap));
    ZeroMemory(buttonStatus, sizeof(byte) * MAX_INPUT_MOUSE);
    ZeroMemory(buttonOldStatus, sizeof(byte) * MAX_INPUT_MOUSE);
    ZeroMemory(buttonMap, sizeof(byte) * MAX_INPUT_MOUSE);
    ZeroMemory(startDblClk, sizeof(ulong) * MAX_INPUT_MOUSE);
    ZeroMemory(buttonCount, sizeof(int) * MAX_INPUT_MOUSE);

	// 더블클릭 체크 시 첫클릭-두번째 클릭까지 제한시간
    timeDblClk = GetDoubleClickTime();
    startDblClk[0] = GetTickCount();

	
    for (int i = 1; i < MAX_INPUT_MOUSE; i++)
        startDblClk[i] = startDblClk[0];


    ulong tLine = 0;

	// 시스템 전역의 파라미터를 가져옴. 여기서는 휠굴림 감지에 사용
    SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &tLine, 0);

    return true;
}

void Input::Update()
{
    memcpy(keyOldState, keyState, sizeof(keyOldState));

    ZeroMemory(keyState, sizeof(keyState));
    ZeroMemory(keyMap, sizeof(keyMap));

    GetKeyboardState(keyState);

	// 키보드 롱탭인지, 단순 입력인지 확인
    for (ulong i = 0; i < MAX_INPUT_KEY; i++)
    {
        byte key = keyState[i] & 0x80;
        keyState[i] = key ? 1 : 0;

        int oldState = keyOldState[i];
        int state = keyState[i];

        if (oldState == 0 && state == 1)
            keyMap[i] = static_cast<uint>(KeyStatus::KEY_INPUT_STATUS_DOWN);  // 이전 0, 현재 1 - KeyDown
        else if (oldState == 1 && state == 0)									 
            keyMap[i] = static_cast<uint>(KeyStatus::KEY_INPUT_STATUS_UP);	  // 이전 1, 현재 0 - KeyUp
        else if (oldState == 1 && state == 1)									 
            keyMap[i] = static_cast<uint>(KeyStatus::KEY_INPUT_STATUS_PRESS); // 이전 1, 현재 1 - KeyPress
        else
            keyMap[i] = static_cast<uint>(KeyStatus::KEY_INPUT_STATUS_NONE);  // 이전 0, 현재 0
    }

	// 255개의 값 모두 복사 (currentStatus to oldStatus)
    memcpy(buttonOldStatus, buttonStatus, sizeof(byte) * MAX_INPUT_MOUSE);

	//currentStatus 0으로 초기화.
    ZeroMemory(buttonStatus, sizeof(byte) * MAX_INPUT_MOUSE);
    ZeroMemory(buttonMap, sizeof(byte) * MAX_INPUT_MOUSE);

	// 눌린 상태라면 1, 안눌린 상태라면 0
    buttonStatus[0] = GetAsyncKeyState(VK_LBUTTON) & 0x8000 ? 1 : 0; // 좌클릭
    buttonStatus[1] = GetAsyncKeyState(VK_RBUTTON) & 0x8000 ? 1 : 0; // 우클릭
    buttonStatus[2] = GetAsyncKeyState(VK_MBUTTON) & 0x8000 ? 1 : 0; // 휠클릭


	// 드래그인지 클릭인지 확인
    for (ulong i = 0; i < MAX_INPUT_MOUSE; i++)
    {
        int tOldStatus = buttonOldStatus[i];
        int tStatus = buttonStatus[i];

        if (tOldStatus == 0 && tStatus == 1)
            buttonMap[i] = static_cast<uint>(ButtonStatus::BUTTON_INPUT_STATUS_DOWN);  // 이전 0, 현재 1 - KeyDown
		else if (tOldStatus == 1 && tStatus == 0)									   
			buttonMap[i] = static_cast<uint>(ButtonStatus::BUTTON_INPUT_STATUS_UP);	   // 이전 1, 현재 0 - KeyUp
		else if (tOldStatus == 1 && tStatus == 1)									   
			buttonMap[i] = static_cast<uint>(ButtonStatus::BUTTON_INPUT_STATUS_PRESS); // 이전 1, 현재 1 - KeyPress (Drag)
		else																		   
			buttonMap[i] = static_cast<uint>(ButtonStatus::BUTTON_INPUT_STATUS_NONE);  // 이전 0, 현재 0
    }

/*************************마우스 보여줌*************************/
    POINT point = { 0, 0 };
    GetCursorPos(&point);
    ScreenToClient(Settings::Get().GetWindowHandle(), &point);

/**************************마우스이동***************************/
    wheelOldStatus.x = wheelStatus.x;
    wheelOldStatus.y = wheelStatus.y;

    wheelStatus.x = static_cast<float>(point.x);
    wheelStatus.y = static_cast<float>(point.y);

    wheelMoveValue = wheelStatus - wheelOldStatus; // 마우스 이동 값 저장

    wheelOldStatus.z = wheelStatus.z; // Z축..?
	
/**************************더블클릭 체크************************/
    ulong tButtonStatus = GetTickCount();
    for (ulong i = 0; i < MAX_INPUT_MOUSE; i++)
    {
        if (buttonMap[i] == static_cast<uint>(ButtonStatus::BUTTON_INPUT_STATUS_DOWN)) // 버튼 DOWN이면
        {
            if (buttonCount[i] == 1)
            {																		   // 현재 누른 시간과 이전에 누른 시간의 차가 일정 시간을 넘길 경우 count 초기화
                if ((tButtonStatus - startDblClk[i]) >= timeDblClk)
                    buttonCount[i] = 0;
            }
            buttonCount[i]++;														   // 아니면 클릭횟수 1 증가

            if (buttonCount[i] == 1)
                startDblClk[i] = tButtonStatus;
        }

        if (buttonMap[i] == static_cast<uint>(ButtonStatus::BUTTON_INPUT_STATUS_UP))   // 버튼 UP상태이면
        {
            if (buttonCount[i] == 1)
            {
                if ((tButtonStatus - startDblClk[i]) >= timeDblClk)					   // 현재 누른 시간과 이전에 누른 시간의 차가 일정 시간을 넘길 경우 count 초기화
                    buttonCount[i] = 0;												   // DOWN에서 증가시켰으므로 UP에서는 증가시키진 않음.
            }
            else if (buttonCount[i] == 2)
            {
                if ((tButtonStatus - startDblClk[i]) <= timeDblClk)                    // 일정 시간 이내에 두번째 클릭이 이뤄졌을 경우
                    buttonMap[i] = static_cast<uint>(ButtonStatus::BUTTON_INPUT_STATUS_DBLCLK); // 얘 더블클릭(더블탭) 했다!

                buttonCount[i] = 0;													   // count 0으로 초기화. 하지 않으면 한번 더블클릭 하면 더이상 더블클릭 안됨.
            }
        }//if
    }//for(i)
}
