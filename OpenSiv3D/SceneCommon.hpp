# pragma once
# include <Siv3D.hpp>

// シーン名
enum class State
{
	Visualizer,
	Executer,
};

class ISState
{
public:
	virtual void initialize() = 0;
	virtual void update() = 0;
	virtual void draw() const = 0;
	virtual State getNextState() const = 0;
	virtual void resetState() = 0;

	virtual ~ISState() {}
};

class StateManager
{
public:
	Array<std::unique_ptr<ISState>> states;

	void update()
	{
		if (m_nowState >= 0 && states.size() > m_nowState)
		{
			states.at(m_nowState)->update();
			states.at(m_nowState)->draw();

			{
				auto s = states.at(m_nowState)->getNextState();
				states.at(m_nowState)->resetState();
				SetState(s);
			}
		}
	}

private:
	int32 m_nowState = 0;
	void SetState(State s)
	{
		int32 num = FromEnum(s);
		if (num >= 0 && states.size() > num)
		{
			m_nowState = num;
		}
	}
};

