#pragma once

#include "stdafx.h"
#include "resource.h"

using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Windows::Forms;
using namespace System::Data;
using namespace System::Drawing;


namespace MainPath {

	/// <summary>
	/// FormLaw 的摘要
	///
	/// 警告: 如果您變更這個類別的名稱，就必須變更與這個類別所依據之所有 .resx 檔案關聯的
	///          Managed 資源編譯器工具的 'Resource File Name' 屬性。
	///          否則，這些設計工具
	///          將無法與這個表單關聯的當地語系化資源
	///          正確互動。
	/// </summary>
	public ref class FormLaw : public System::Windows::Forms::Form
	{
	public:
		FormLaw(void)
		{
			InitializeComponent();
			//
			//TODO: 在此加入建構函式程式碼
			//
		}

		int CitationFileProvided;
		int POSexamined;
		int POSdiscussed;
		int POScited;
		int POSmentioned;
		int NEG1star;
		int NEG2star;
		int NEG3star;
		int NEG4star;
		int TakeWeight;

	private: System::Windows::Forms::Label^  label2;
	public: 
	private: System::Windows::Forms::Label^  label1;
	private: System::Windows::Forms::Label^  label3;
	private: System::Windows::Forms::RadioButton^  radioButton4;
	private: System::Windows::Forms::RadioButton^  radioButton3;
	private: System::Windows::Forms::RadioButton^  radioButton6;
	private: System::Windows::Forms::RadioButton^  radioButton5;
	private: System::Windows::Forms::CheckBox^  checkBox8;
	private: System::Windows::Forms::CheckBox^  checkBox7;
	private: System::Windows::Forms::CheckBox^  checkBox6;
	private: System::Windows::Forms::Button^  button1;

	protected:
		/// <summary>
		/// 清除任何使用中的資源。
		/// </summary>
		~FormLaw()
		{
			if (components)
			{
				delete components;
			}
		}
	private: System::Windows::Forms::GroupBox^  groupBox1;
	protected: 
	private: System::Windows::Forms::CheckBox^  checkBox5;
	private: System::Windows::Forms::CheckBox^  checkBox4;
	private: System::Windows::Forms::CheckBox^  checkBox3;
	private: System::Windows::Forms::CheckBox^  checkBox2;
	private: System::Windows::Forms::CheckBox^  checkBox1;
	private: System::Windows::Forms::GroupBox^  groupBox2;
	private: System::Windows::Forms::RadioButton^  radioButton2;
	private: System::Windows::Forms::RadioButton^  radioButton1;

	private:
		/// <summary>
		/// 設計工具所需的變數。
		/// </summary>
		System::ComponentModel::Container ^components;

#pragma region Windows Form Designer generated code
		/// <summary>
		/// 此為設計工具支援所需的方法 - 請勿使用程式碼編輯器修改這個方法的內容。
		///
		/// </summary>
		void InitializeComponent(void)
		{
			this->groupBox1 = (gcnew System::Windows::Forms::GroupBox());
			this->checkBox8 = (gcnew System::Windows::Forms::CheckBox());
			this->checkBox7 = (gcnew System::Windows::Forms::CheckBox());
			this->checkBox6 = (gcnew System::Windows::Forms::CheckBox());
			this->label3 = (gcnew System::Windows::Forms::Label());
			this->label2 = (gcnew System::Windows::Forms::Label());
			this->checkBox5 = (gcnew System::Windows::Forms::CheckBox());
			this->checkBox4 = (gcnew System::Windows::Forms::CheckBox());
			this->checkBox3 = (gcnew System::Windows::Forms::CheckBox());
			this->checkBox2 = (gcnew System::Windows::Forms::CheckBox());
			this->checkBox1 = (gcnew System::Windows::Forms::CheckBox());
			this->groupBox2 = (gcnew System::Windows::Forms::GroupBox());
			this->radioButton6 = (gcnew System::Windows::Forms::RadioButton());
			this->radioButton5 = (gcnew System::Windows::Forms::RadioButton());
			this->radioButton4 = (gcnew System::Windows::Forms::RadioButton());
			this->radioButton3 = (gcnew System::Windows::Forms::RadioButton());
			this->label1 = (gcnew System::Windows::Forms::Label());
			this->radioButton2 = (gcnew System::Windows::Forms::RadioButton());
			this->radioButton1 = (gcnew System::Windows::Forms::RadioButton());
			this->button1 = (gcnew System::Windows::Forms::Button());
			this->groupBox1->SuspendLayout();
			this->groupBox2->SuspendLayout();
			this->SuspendLayout();
			// 
			// groupBox1
			// 
			this->groupBox1->Controls->Add(this->checkBox8);
			this->groupBox1->Controls->Add(this->checkBox7);
			this->groupBox1->Controls->Add(this->checkBox6);
			this->groupBox1->Controls->Add(this->label3);
			this->groupBox1->Controls->Add(this->label2);
			this->groupBox1->Controls->Add(this->checkBox5);
			this->groupBox1->Controls->Add(this->checkBox4);
			this->groupBox1->Controls->Add(this->checkBox3);
			this->groupBox1->Controls->Add(this->checkBox2);
			this->groupBox1->Controls->Add(this->checkBox1);
			this->groupBox1->Location = System::Drawing::Point(29, 29);
			this->groupBox1->Name = L"groupBox1";
			this->groupBox1->Size = System::Drawing::Size(274, 271);
			this->groupBox1->TabIndex = 0;
			this->groupBox1->TabStop = false;
			this->groupBox1->Text = L"KEYCITE Infornation Selection";
			this->groupBox1->Enter += gcnew System::EventHandler(this, &FormLaw::groupBox1_Enter);
			// 
			// checkBox8
			// 
			this->checkBox8->AutoSize = true;
			this->checkBox8->Location = System::Drawing::Point(39, 237);
			this->checkBox8->Name = L"checkBox8";
			this->checkBox8->Size = System::Drawing::Size(107, 16);
			this->checkBox8->TabIndex = 9;
			this->checkBox8->Text = L"Negative cases - *";
			this->checkBox8->UseVisualStyleBackColor = true;
			this->checkBox8->CheckedChanged += gcnew System::EventHandler(this, &FormLaw::checkBox8_CheckedChanged);
			// 
			// checkBox7
			// 
			this->checkBox7->AutoSize = true;
			this->checkBox7->Location = System::Drawing::Point(39, 215);
			this->checkBox7->Name = L"checkBox7";
			this->checkBox7->Size = System::Drawing::Size(113, 16);
			this->checkBox7->TabIndex = 8;
			this->checkBox7->Text = L"Negative cases - **";
			this->checkBox7->UseVisualStyleBackColor = true;
			this->checkBox7->CheckedChanged += gcnew System::EventHandler(this, &FormLaw::checkBox7_CheckedChanged);
			// 
			// checkBox6
			// 
			this->checkBox6->AutoSize = true;
			this->checkBox6->Location = System::Drawing::Point(39, 193);
			this->checkBox6->Name = L"checkBox6";
			this->checkBox6->Size = System::Drawing::Size(119, 16);
			this->checkBox6->TabIndex = 7;
			this->checkBox6->Text = L"Negative cases - ***";
			this->checkBox6->UseVisualStyleBackColor = true;
			this->checkBox6->CheckedChanged += gcnew System::EventHandler(this, &FormLaw::checkBox6_CheckedChanged);
			// 
			// label3
			// 
			this->label3->AutoSize = true;
			this->label3->Location = System::Drawing::Point(21, 22);
			this->label3->Name = L"label3";
			this->label3->Size = System::Drawing::Size(229, 24);
			this->label3->TabIndex = 6;
			this->label3->Text = L"Citation file is not provided. Will use KEYCITE\n information to generate citation" 
				L".";
			// 
			// label2
			// 
			this->label2->AutoSize = true;
			this->label2->Location = System::Drawing::Point(21, 59);
			this->label2->Name = L"label2";
			this->label2->Size = System::Drawing::Size(164, 12);
			this->label2->TabIndex = 5;
			this->label2->Text = L"KEYCITE information to include:";
			// 
			// checkBox5
			// 
			this->checkBox5->AutoSize = true;
			this->checkBox5->Location = System::Drawing::Point(39, 172);
			this->checkBox5->Name = L"checkBox5";
			this->checkBox5->Size = System::Drawing::Size(125, 16);
			this->checkBox5->TabIndex = 4;
			this->checkBox5->Text = L"Negative cases - ****";
			this->checkBox5->UseVisualStyleBackColor = true;
			this->checkBox5->CheckedChanged += gcnew System::EventHandler(this, &FormLaw::checkBox5_CheckedChanged);
			// 
			// checkBox4
			// 
			this->checkBox4->AutoSize = true;
			this->checkBox4->Location = System::Drawing::Point(39, 150);
			this->checkBox4->Name = L"checkBox4";
			this->checkBox4->Size = System::Drawing::Size(184, 16);
			this->checkBox4->TabIndex = 3;
			this->checkBox4->Text = L"Positive cases  - *       (Mentioned)";
			this->checkBox4->UseVisualStyleBackColor = true;
			this->checkBox4->CheckedChanged += gcnew System::EventHandler(this, &FormLaw::checkBox4_CheckedChanged);
			// 
			// checkBox3
			// 
			this->checkBox3->AutoSize = true;
			this->checkBox3->Location = System::Drawing::Point(39, 128);
			this->checkBox3->Name = L"checkBox3";
			this->checkBox3->Size = System::Drawing::Size(159, 16);
			this->checkBox3->TabIndex = 2;
			this->checkBox3->Text = L"Positive cases  - **     (Cited)";
			this->checkBox3->UseVisualStyleBackColor = true;
			this->checkBox3->CheckedChanged += gcnew System::EventHandler(this, &FormLaw::checkBox3_CheckedChanged);
			// 
			// checkBox2
			// 
			this->checkBox2->AutoSize = true;
			this->checkBox2->Location = System::Drawing::Point(39, 106);
			this->checkBox2->Name = L"checkBox2";
			this->checkBox2->Size = System::Drawing::Size(175, 16);
			this->checkBox2->TabIndex = 1;
			this->checkBox2->Text = L"Positive cases  - ***   (Discused)";
			this->checkBox2->UseVisualStyleBackColor = true;
			this->checkBox2->CheckedChanged += gcnew System::EventHandler(this, &FormLaw::checkBox2_CheckedChanged);
			// 
			// checkBox1
			// 
			this->checkBox1->AutoSize = true;
			this->checkBox1->Location = System::Drawing::Point(39, 83);
			this->checkBox1->Name = L"checkBox1";
			this->checkBox1->Size = System::Drawing::Size(181, 16);
			this->checkBox1->TabIndex = 0;
			this->checkBox1->Text = L"Positive cases  - **** (Examined)";
			this->checkBox1->UseVisualStyleBackColor = true;
			this->checkBox1->CheckedChanged += gcnew System::EventHandler(this, &FormLaw::checkBox1_CheckedChanged);
			// 
			// groupBox2
			// 
			this->groupBox2->Controls->Add(this->radioButton6);
			this->groupBox2->Controls->Add(this->radioButton5);
			this->groupBox2->Controls->Add(this->radioButton4);
			this->groupBox2->Controls->Add(this->radioButton3);
			this->groupBox2->Controls->Add(this->label1);
			this->groupBox2->Controls->Add(this->radioButton2);
			this->groupBox2->Controls->Add(this->radioButton1);
			this->groupBox2->Location = System::Drawing::Point(329, 29);
			this->groupBox2->Name = L"groupBox2";
			this->groupBox2->Size = System::Drawing::Size(340, 209);
			this->groupBox2->TabIndex = 1;
			this->groupBox2->TabStop = false;
			this->groupBox2->Text = L"Citation Level Consideration";
			this->groupBox2->Enter += gcnew System::EventHandler(this, &FormLaw::groupBox2_Enter);
			// 
			// radioButton6
			// 
			this->radioButton6->AutoSize = true;
			this->radioButton6->Location = System::Drawing::Point(32, 167);
			this->radioButton6->Name = L"radioButton6";
			this->radioButton6->Size = System::Drawing::Size(298, 16);
			this->radioButton6->TabIndex = 6;
			this->radioButton6->TabStop = true;
			this->radioButton6->Text = L"Exponential (20.09, 7.39, 2.72, 1; -20.09, -7.39, -2.72, -1)";
			this->radioButton6->UseVisualStyleBackColor = true;
			this->radioButton6->CheckedChanged += gcnew System::EventHandler(this, &FormLaw::radioButton6_CheckedChanged);
			// 
			// radioButton5
			// 
			this->radioButton5->AutoSize = true;
			this->radioButton5->Location = System::Drawing::Point(32, 145);
			this->radioButton5->Name = L"radioButton5";
			this->radioButton5->Size = System::Drawing::Size(170, 16);
			this->radioButton5->TabIndex = 5;
			this->radioButton5->TabStop = true;
			this->radioButton5->Text = L"Linear (4, 3, 2, 1; -4, -3, -2, -1)";
			this->radioButton5->UseVisualStyleBackColor = true;
			this->radioButton5->CheckedChanged += gcnew System::EventHandler(this, &FormLaw::radioButton5_CheckedChanged);
			// 
			// radioButton4
			// 
			this->radioButton4->AutoSize = true;
			this->radioButton4->Location = System::Drawing::Point(32, 123);
			this->radioButton4->Name = L"radioButton4";
			this->radioButton4->Size = System::Drawing::Size(157, 16);
			this->radioButton4->TabIndex = 4;
			this->radioButton4->TabStop = true;
			this->radioButton4->Text = L"Flat (1, 1, 1, 1; -1, -1, -1, -1)";
			this->radioButton4->UseVisualStyleBackColor = true;
			this->radioButton4->CheckedChanged += gcnew System::EventHandler(this, &FormLaw::radioButton4_CheckedChanged);
			// 
			// radioButton3
			// 
			this->radioButton3->AutoSize = true;
			this->radioButton3->Location = System::Drawing::Point(32, 101);
			this->radioButton3->Name = L"radioButton3";
			this->radioButton3->Size = System::Drawing::Size(282, 16);
			this->radioButton3->TabIndex = 3;
			this->radioButton3->TabStop = true;
			this->radioButton3->Text = L"Exponential (20.09, 7.39, 2.72, 1; +20.09, +7.39, +2.72, +1)";
			this->radioButton3->UseVisualStyleBackColor = true;
			this->radioButton3->CheckedChanged += gcnew System::EventHandler(this, &FormLaw::radioButton3_CheckedChanged);
			// 
			// label1
			// 
			this->label1->AutoSize = true;
			this->label1->Location = System::Drawing::Point(18, 22);
			this->label1->Name = L"label1";
			this->label1->Size = System::Drawing::Size(160, 24);
			this->label1->TabIndex = 2;
			this->label1->Text = L"Method of handling citation level\n for searching mainpath\?";
			// 
			// radioButton2
			// 
			this->radioButton2->AutoSize = true;
			this->radioButton2->Location = System::Drawing::Point(32, 79);
			this->radioButton2->Name = L"radioButton2";
			this->radioButton2->Size = System::Drawing::Size(154, 16);
			this->radioButton2->TabIndex = 1;
			this->radioButton2->TabStop = true;
			this->radioButton2->Text = L"Linear (4, 3, 2, 1; +4, +3, +2, +1)";
			this->radioButton2->UseVisualStyleBackColor = true;
			this->radioButton2->CheckedChanged += gcnew System::EventHandler(this, &FormLaw::radioButton2_CheckedChanged);
			// 
			// radioButton1
			// 
			this->radioButton1->AutoSize = true;
			this->radioButton1->Location = System::Drawing::Point(32, 57);
			this->radioButton1->Name = L"radioButton1";
			this->radioButton1->Size = System::Drawing::Size(141, 16);
			this->radioButton1->TabIndex = 0;
			this->radioButton1->TabStop = true;
			this->radioButton1->Text = L"Flat (1, 1, 1, 1; +1, +1, +1, +1)";
			this->radioButton1->UseVisualStyleBackColor = true;
			this->radioButton1->CheckedChanged += gcnew System::EventHandler(this, &FormLaw::radioButton1_CheckedChanged);
			// 
			// button1
			// 
			this->button1->Location = System::Drawing::Point(417, 277);
			this->button1->Name = L"button1";
			this->button1->Size = System::Drawing::Size(174, 23);
			this->button1->TabIndex = 2;
			this->button1->Text = L"Continue";
			this->button1->UseVisualStyleBackColor = true;
			this->button1->Click += gcnew System::EventHandler(this, &FormLaw::button1_Click);
			// 
			// FormLaw
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 12);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(696, 319);
			this->Controls->Add(this->button1);
			this->Controls->Add(this->groupBox2);
			this->Controls->Add(this->groupBox1);
			this->Name = L"FormLaw";
			this->Text = L"KEYCITE Information Selection Menu";
			this->Load += gcnew System::EventHandler(this, &FormLaw::FormLaw_Load);
			this->groupBox1->ResumeLayout(false);
			this->groupBox1->PerformLayout();
			this->groupBox2->ResumeLayout(false);
			this->groupBox2->PerformLayout();
			this->ResumeLayout(false);

		}
#pragma endregion
private: System::Void checkBox1_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
			 if (POSexamined == 1)
				 POSexamined = 0;
			 else
				 POSexamined = 1;
		 }
private: System::Void checkBox2_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
			 if (POSdiscussed == 1)
				 POSdiscussed = 0;
			 else
				 POSdiscussed = 1;
		 }
private: System::Void checkBox3_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
			 if (POScited == 1)
				 POScited = 0;
			 else
				 POScited = 1;
		 }
private: System::Void checkBox4_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
			 if (POSmentioned == 1)
				 POSmentioned = 0;
			 else
				 POSmentioned = 1;
		 }
private: System::Void checkBox5_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
			 if (NEG4star == 1)
				 NEG4star = 0;
			 else
				 NEG4star = 1;
		 }
private: System::Void button1_Click(System::Object^  sender, System::EventArgs^  e) {
			 this->Close();
		 }
private: System::Void radioButton1_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
			  TakeWeight = FLAT_IGNORE_SIGN;
		 }
private: System::Void radioButton2_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
			  TakeWeight = LINEAR_IGNORE_SIGN;
		 }
private: System::Void groupBox1_Enter(System::Object^  sender, System::EventArgs^  e) {
		 }
private: System::Void groupBox2_Enter(System::Object^  sender, System::EventArgs^  e) {
		 }
private: System::Void FormLaw_Load(System::Object^  sender, System::EventArgs^  e) {
			 checkBox1->Checked = true;		POSexamined = 1;
			 checkBox2->Checked = true;		POSdiscussed = 1;
			 checkBox3->Checked = true;		POScited = 1;
			 checkBox4->Checked = true;		POSmentioned = 1;	// 2013/02/19, changed from false to true, and from 0 to 1
			 checkBox5->Checked = true;		NEG4star = 1;	
			 checkBox6->Checked = true;		NEG3star = 1;
			 checkBox7->Checked = true;		NEG2star = 1;	
			 checkBox8->Checked = true;		NEG1star = 1;		// 2013/02/19, changed from false to true, and from 0 to 1					 	
			 radioButton2->Checked = true;	TakeWeight = LINEAR_IGNORE_SIGN;
			 if (CitationFileProvided)
				 groupBox1->Enabled = false;
			 else
				 groupBox1->Enabled = true;
		 }
private: System::Void radioButton3_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
			 TakeWeight = EXPONENTIAL_IGNORE_SIGN;
		 }
private: System::Void radioButton4_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
			 TakeWeight = FLAT_TAKE_SIGN;
		 }
private: System::Void radioButton5_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
			 TakeWeight = LINEAR_TAKE_SIGN;
		 }
private: System::Void radioButton6_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
			 TakeWeight = EXPONENTIAL_TAKE_SIGN;
		 }
private: System::Void checkBox6_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
			 if (NEG3star == 1)
				 NEG3star = 0;
			 else
				 NEG3star = 1;
		 }
private: System::Void checkBox7_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
			 if (NEG2star == 1)
				 NEG2star = 0;
			 else
				 NEG2star = 1;
		 }
private: System::Void checkBox8_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
			 if (NEG1star == 1)
				 NEG1star = 0;
			 else
				 NEG1star = 1;
		 }
};
}
