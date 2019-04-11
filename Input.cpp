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
    MouseProc = std::bind // �Լ�ȣ���� �̸� �����س���
    (
        &Input::MsgProc,
        this,
        std::placeholders::_1,
        std::placeholders::_2,
        std::placeholders::_3,
        std::placeholders::_4   // �޽����� ���� �����Ƿ� ������ �̸� �޾Ƽ� ����������.
    );

    EventSystem::Get().Subscribe(EventType::Event_Update, EVENT_HANDLER(Update));
}

// ������Ͽ� �����
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
	// �迭 0���� �ʱ�ȭ
    ZeroMemory(keyState, sizeof(keyState));
    ZeroMemory(keyOldState, sizeof(keyOldState));
    ZeroMemory(keyMap, sizeof(keyMap));
    ZeroMemory(buttonStatus, sizeof(byte) * MAX_INPUT_MOUSE);
    ZeroMemory(buttonOldStatus, sizeof(byte) * MAX_INPUT_MOUSE);
    ZeroMemory(buttonMap, sizeof(byte) * MAX_INPUT_MOUSE);
    ZeroMemory(startDblClk, sizeof(ulong) * MAX_INPUT_MOUSE);
    ZeroMemory(buttonCount, sizeof(int) * MAX_INPUT_MOUSE);

	// ����Ŭ�� üũ �� ùŬ��-�ι�° Ŭ������ ���ѽð�
    timeDblClk = GetDoubleClickTime();
    startDblClk[0] = GetTickCount();

	
    for (int i = 1; i < MAX_INPUT_MOUSE; i++)
        startDblClk[i] = startDblClk[0];


    ulong tLine = 0;

	// �ý��� ������ �Ķ���͸� ������. ���⼭�� �ٱ��� ������ ���
    SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &tLine, 0);

    return true;
}

void Input::Update()
{
    memcpy(keyOldState, keyState, sizeof(keyOldState));

    ZeroMemory(keyState, sizeof(keyState));
    ZeroMemory(keyMap, sizeof(keyMap));

    GetKeyboardState(keyState);

	// Ű���� ��������, �ܼ� �Է����� Ȯ��
    for (ulong i = 0; i < MAX_INPUT_KEY; i++)
    {
        byte key = keyState[i] & 0x80;
        keyState[i] = key ? 1 : 0;

        int oldState = keyOldState[i];
        int state = keyState[i];

        if (oldState == 0 && state == 1)
            keyMap[i] = static_cast<uint>(KeyStatus::KEY_INPUT_STATUS_DOWN);  // ���� 0, ���� 1 - KeyDown
        else if (oldState == 1 && state == 0)									 
            keyMap[i] = static_cast<uint>(KeyStatus::KEY_INPUT_STATUS_UP);	  // ���� 1, ���� 0 - KeyUp
        else if (oldState == 1 && state == 1)									 
            keyMap[i] = static_cast<uint>(KeyStatus::KEY_INPUT_STATUS_PRESS); // ���� 1, ���� 1 - KeyPress
        else
            keyMap[i] = static_cast<uint>(KeyStatus::KEY_INPUT_STATUS_NONE);  // ���� 0, ���� 0
    }

	// 255���� �� ��� ���� (currentStatus to oldStatus)
    memcpy(buttonOldStatus, buttonStatus, sizeof(byte) * MAX_INPUT_MOUSE);

	//currentStatus 0���� �ʱ�ȭ.
    ZeroMemory(buttonStatus, sizeof(byte) * MAX_INPUT_MOUSE);
    ZeroMemory(buttonMap, sizeof(byte) * MAX_INPUT_MOUSE);

	// ���� ���¶�� 1, �ȴ��� ���¶�� 0
    buttonStatus[0] = GetAsyncKeyState(VK_LBUTTON) & 0x8000 ? 1 : 0; // ��Ŭ��
    buttonStatus[1] = GetAsyncKeyState(VK_RBUTTON) & 0x8000 ? 1 : 0; // ��Ŭ��
    buttonStatus[2] = GetAsyncKeyState(VK_MBUTTON) & 0x8000 ? 1 : 0; // ��Ŭ��


	// �巡������ Ŭ������ Ȯ��
    for (ulong i = 0; i < MAX_INPUT_MOUSE; i++)
    {
        int tOldStatus = buttonOldStatus[i];
        int tStatus = buttonStatus[i];

        if (tOldStatus == 0 && tStatus == 1)
            buttonMap[i] = static_cast<uint>(ButtonStatus::BUTTON_INPUT_STATUS_DOWN);  // ���� 0, ���� 1 - KeyDown
		else if (tOldStatus == 1 && tStatus == 0)									   
			buttonMap[i] = static_cast<uint>(ButtonStatus::BUTTON_INPUT_STATUS_UP);	   // ���� 1, ���� 0 - KeyUp
		else if (tOldStatus == 1 && tStatus == 1)									   
			buttonMap[i] = static_cast<uint>(ButtonStatus::BUTTON_INPUT_STATUS_PRESS); // ���� 1, ���� 1 - KeyPress (Drag)
		else																		   
			buttonMap[i] = static_cast<uint>(ButtonStatus::BUTTON_INPUT_STATUS_NONE);  // ���� 0, ���� 0
    }

/*************************���콺 ������*************************/
    POINT point = { 0, 0 };
    GetCursorPos(&point);
    ScreenToClient(Settings::Get().GetWindowHandle(), &point);

/**************************���콺�̵�***************************/
    wheelOldStatus.x = wheelStatus.x;
    wheelOldStatus.y = wheelStatus.y;

    wheelStatus.x = static_cast<float>(point.x);
    wheelStatus.y = static_cast<float>(point.y);

    wheelMoveValue = wheelStatus - wheelOldStatus;

    wheelOldStatus.z = wheelStatus.z; // �ٱ��� �� ����
	
/**************************����Ŭ�� üũ************************/
    ulong tButtonStatus = GetTickCount();
    for (ulong i = 0; i < MAX_INPUT_MOUSE; i++)
    {
        if (buttonMap[i] == static_cast<uint>(ButtonStatus::BUTTON_INPUT_STATUS_DOWN)) // ��ư DOWN�̸�
        {
            if (buttonCount[i] == 1)
            {																		   // ���� ���� �ð��� ������ ���� �ð��� ���� ���� �ð��� �ѱ� ��� count �ʱ�ȭ
                if ((tButtonStatus - startDblClk[i]) >= timeDblClk)
                    buttonCount[i] = 0;
            }
            buttonCount[i]++;														   // �ƴϸ� Ŭ��Ƚ�� 1 ����

            if (buttonCount[i] == 1)
                startDblClk[i] = tButtonStatus;
        }

        if (buttonMap[i] == static_cast<uint>(ButtonStatus::BUTTON_INPUT_STATUS_UP))   // ��ư UP�����̸�
        {
            if (buttonCount[i] == 1)
            {
                if ((tButtonStatus - startDblClk[i]) >= timeDblClk)					   // ���� ���� �ð��� ������ ���� �ð��� ���� ���� �ð��� �ѱ� ��� count �ʱ�ȭ
                    buttonCount[i] = 0;												   // DOWN���� �����������Ƿ� UP������ ������Ű�� ����.
            }
            else if (buttonCount[i] == 2)
            {
                if ((tButtonStatus - startDblClk[i]) <= timeDblClk)                    // ���� �ð� �̳��� �ι�° Ŭ���� �̷����� ���
                    buttonMap[i] = static_cast<uint>(ButtonStatus::BUTTON_INPUT_STATUS_DBLCLK); // �� ����Ŭ��(������) �ߴ�!

                buttonCount[i] = 0;													   // count 0���� �ʱ�ȭ. ���� ������ �ѹ� ����Ŭ�� �ϸ� ���̻� ����Ŭ�� �ȵ�.
            }
        }//if
    }//for(i)
}
