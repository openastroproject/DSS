#pragma once
#include "DSSProgress.h"

namespace Ui {
	class ProgressDlg;
}

namespace DSS
{
	class ProgressConsole : public ProgressBase
	{
	private:
		QTextStream m_out;
		TERMINAL_OUTPUT_MODE m_style;
		QString m_strLastSent[3];

	public:
		ProgressConsole(TERMINAL_OUTPUT_MODE mode) :
			ProgressBase(),
			m_out(stdout),
			m_style(mode)
		{
		}
		virtual ~ProgressConsole()
		{
			Close();
		}

		virtual void initialise() override{};
		virtual void applyStart1Text(const QString& strText) override { PrintText(strText, OT_TEXT1); }
		virtual void applyStart2Text(const QString& strText) override 
		{
			if(m_total2>0)
				PrintText(strText, OT_TEXT2); 
		}
		virtual void applyProgress1([[maybe_unused]] int lAchieved) override
		{
			PrintText(GetProgress1Text(), OT_PROGRESS1);
		}
		virtual void applyProgress2([[maybe_unused]] int lAchieved) override
		{
			PrintText(GetProgress2Text(), OT_PROGRESS2);
		}
		virtual void applyTitleText(const QString& strText) override { PrintText(strText, OT_TITLE); }

		virtual void endProgress2() override {}
		virtual bool hasBeenCanceled() override { return false; }
		virtual void closeProgress() { }
		virtual bool doWarning([[maybe_unused]] const QString& szText) override { return true; }
		virtual void applyProcessorsUsed([[maybe_unused]] int nCount) override {};

	private:
		void PrintText(const QString& szText, eOutputType type)
		{
			QString singleLineText(szText);
			singleLineText.replace('\n', ' ');

			switch (m_style)
			{
			case TERMINAL_OUTPUT_MODE::BASIC:
				PrintBasic(singleLineText, type, false);
				break;
			case TERMINAL_OUTPUT_MODE::COLOURED:
				PrintBasic(singleLineText, type, true);
				break;
			case TERMINAL_OUTPUT_MODE::FORMATTED:
				PrintFormatted(singleLineText, type);
				break;
			}
		}
		void PrintBasic(const QString& szText, eOutputType type, bool bColour)
		{
			switch(type)
			{
			case OT_TITLE:
				if (m_strLastSent[0].compare(szText) != 0)
				{
					m_out << (bColour ? "\033[036m" : "") << szText << "\n";
					m_strLastSent[0] = szText;
				}
				break;
			case OT_TEXT1:
				if (m_strLastSent[1].compare(szText) != 0)
				{
					// Don't echo out if the same as the title text.
					if (m_strLastSent[1].compare(m_strLastSent[0]) != 0)
						m_out << (bColour ? "\033[1;33m" : "") << szText << "\n";
					m_strLastSent[1] = szText;
				}
				break;
			case OT_TEXT2:
				if (m_strLastSent[2].compare(szText) != 0)
				{
					// Don't echo out if the same as the detail text
					if(m_strLastSent[2].compare(m_strLastSent[1]) != 0)
						m_out << (bColour ? "\033[1;33m" : "") << szText << "\n";
					m_strLastSent[2] = szText;
				}
					
				break;
			case OT_PROGRESS1:
			case OT_PROGRESS2:
				m_out << (bColour ? "\033[32m" : "") << szText << "\r";
				break;
			}
			m_out.flush();
		}
		void PrintFormatted(const QString& szText, [[maybe_unused]] eOutputType type)
		{
			m_out << "\033[7;0H";
			m_out << "\033[036m--------------------------------------------------------------------------------";
			m_out << "\033[8;0H";
			m_out << "\033[2K";
			m_out << "\033[036m" << GetTitleText();
			m_out << "\033[9;0H";
			m_out << "\033[036m--------------------------------------------------------------------------------";
			m_out << "\033[10;0H";
			m_out << "\033[2K";
			m_out << "\033[1;33m" << GetStart1Text();
			m_out << "\033[11;0H";
			m_out << "\033[2K";
			m_out << "\033[1;33m" << GetProgress1Text();
			m_out << "\033[12;0H";
			m_out << "\033[2K";
			m_out << "\033[32m" << (m_jointProgress ? "" : GetStart2Text());
			m_out << "\033[13;0H";
			m_out << "\033[2K";
			m_out << "\033[32m" << (m_jointProgress ? "" : GetProgress2Text());
			m_out << "\033[14;0H";
			m_out << "\033[036m--------------------------------------------------------------------------------";
			m_out << "\033[15;0H";
			//m_out << "\033[2K";
			m_out.flush();
		}
	};
}
