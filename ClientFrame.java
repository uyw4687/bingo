package cn.hw2;

import java.awt.BorderLayout;
import java.awt.Button;
import java.awt.Frame;
import java.awt.GridLayout;
import java.awt.Panel;
import java.awt.TextArea;
import java.awt.TextField;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

public class ClientFrame extends Frame implements ActionListener {

	private static final long serialVersionUID = 2L;
	private TextArea taDisplay;
	private int nextVal = 0;
	private boolean myTurn = false;
	private Button submitBtn;
	public Button [] buttons;
	private int nextSecretVal = 0; // only for Main Culprit
	private TextField tfInput;
	private int [] nums;
	private boolean [] got;
	private int role = 0;
	
	//pick the number and set it invisible
	public void setGot(int n) {
		for(int i = 0; i < 25; i ++)
		{
			if(n == Integer.parseInt(buttons[i].getLabel())) {
				buttons[i].setVisible(false);
				got[n] = true;
				break;
			}
		}
	}
	
	public int getSecretVal() {
		
		return nextSecretVal;
		
	}
	
	//secret value sent or got from private messages
	public void setSecretVal(int secretVal) {
		
		this.nextSecretVal = secretVal;
		if(role == 1)
			this.appendText("Secret Number is " + secretVal + "\n");
		
	}
	
	public void appendText(String text) {
		
		taDisplay.append(text);
		
	}
	
	public void setTurn(boolean isMyTurn) {
		
		myTurn = isMyTurn;
		
	}
	
	public boolean getTurn() {
		
		return myTurn;
		
	}
	
	public int getNextVal() {
		
		int temp = nextVal;
		nextVal = 0;
		
		return temp;
		
	}
	
	public ClientFrame(int [] nums, int role) {
		
		this.nums = nums;
		this.role = role;
		
		got = new boolean[100];
		buttons = new Button[100];
		setLayout(new BorderLayout());
		Panel p = new Panel();
		
		int i = 0;
		while(i < 25)
		{
			buttons[i] = new Button(); 
			buttons[i].setLabel(Integer.toString(nums[i]));
			p.add(buttons[i]);
			buttons[i].addActionListener(this);
			i++;
		}
		
		p.setLayout(new GridLayout(5, 5));
		p.setSize(600, 400);
		add(p, BorderLayout.CENTER);
		
		Panel p2 = new Panel();
		taDisplay = new TextArea(40, 40);
		p2.add(taDisplay);
		
		//Main Culprit needs to send something so needs a textfield and a button
		if(role == 0) {
			
			p2.setLayout(new GridLayout(3, 1));
		
			tfInput = new TextField(10);
			submitBtn = new Button();
			submitBtn.setLabel("Submit");
			submitBtn.addActionListener(this);

			p2.add(tfInput);
			p2.add(submitBtn);
			
		}
		
		add(p2, BorderLayout.EAST);
		
		setTitle("BINGO");
		setLocation(300, 25);
		setSize(1000, 800);
		setVisible(true);
		
	}

	//buttons have listener
	@Override
	public void actionPerformed(ActionEvent e) {
		try
		{
			Button selected = (Button)(e.getSource());
			if(myTurn) {
				//secret number submitted
				if(selected == submitBtn) {
					int temp = Integer.parseInt(tfInput.getText());
					tfInput.setText("");
					
					if(nextSecretVal != 0) {
						this.appendText("Already set secret number\n");
						return;
					}
					
					boolean contains = false;
					
					for(int i : nums) {
						if(i==temp) {
							contains = true;
							break;
						}
					}
					if(contains)
					{
						if(got[temp] == false) {
							nextSecretVal = temp;
							System.out.println("next Secret Value : " + nextSecretVal);
							return;
						}
					}
					this.appendText("Invalid value input\n");
				}
				else {
					if(role == 0) { //main culprit
						if(nextSecretVal != 0) { //already set secret value
							selected.setVisible(false);
							nextVal = Integer.parseInt(selected.getLabel());
							System.out.println("selected : " + nextVal);
							got[nextVal] = true;
							this.setTurn(false);
						}
						else {
							this.appendText("Send message fisrt\n");
						}
					}
					else { //co-partner
						if(nextSecretVal == 0) { //don't have to follow secret value in this case
							selected.setVisible(false);
							nextVal = Integer.parseInt(selected.getLabel());
							got[nextVal] = true;
							System.out.println("selected : " + nextVal);
							this.setTurn(false);
						}
						else {
							if(Integer.parseInt(selected.getLabel()) == nextSecretVal) {
								selected.setVisible(false);
								nextVal = Integer.parseInt(selected.getLabel());
								got[nextVal] = true;
								System.out.println("selected : " + nextVal);
								this.setTurn(false);
								this.nextSecretVal = 0;
							}
							else { //didn't pick the secret number
								this.appendText("You should select the secret Number\n");
							}
						}
					}
				}
			}
			else {
				this.appendText("Not Your Turn\n");
			}
		}catch(NumberFormatException eve) { // malformed number input
			this.appendText("Malformed number\n");
			return;
		}
	}

}
