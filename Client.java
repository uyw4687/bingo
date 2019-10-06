package cn.hw2;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.Socket;
	
public class Client extends Thread {

	private Socket clientSocket;
	private PrintWriter out;
	private BufferedReader in;
	private String ip;
	private int port;
	private String clientID;
	
	public void startConnection() throws IOException { //connect to the server
		
		clientSocket = new Socket(ip, port);
		out = new PrintWriter(clientSocket.getOutputStream(), true);
		in = new BufferedReader(new InputStreamReader(clientSocket.getInputStream()));
		
	}
	
	public String sendMessage(String msg) throws IOException { //send message and get response
		
		this.startConnection();
		out.println(msg);
		String resp = in.readLine();
		System.out.println("Server response : " + resp);
		if(resp.compareTo("Invalid") == 0) {
			System.out.println("Invalid Request Exiting...");
			java.lang.System.exit(0);
		}
		return resp;
		
	}
	
	public void stopConnection() throws IOException {
		
		in.close();
		out.close();
		clientSocket.close();
		
	}
	
	public Client(String ip, int port) {
		this.ip = ip;
		this.port = port;
	}
	
	public static void main(String[] args) throws IOException, InterruptedException {
		Client client = new Client("cn1.snucse.org", 20700);
		client.start();
	}
	public void run() {
		try {
			int role = -1; // 0 : Main Culprit, 1 : Co-partner
		
			//register as a new player
			String response = this.sendMessage("New Player\r\n");
	
			String[] splitResponse = response.split(" ", 3);
			
			if(splitResponse[0].compareTo("Already") == 0)
			{
				System.out.println("Already Full Exiting...");
				java.lang.System.exit(0);
			}
			
			if(splitResponse[2].compareTo("MainCulprit") == 0)
				role = 0;
			else if(splitResponse[2].compareTo("CoPartner") == 0)
				role = 1;
			else
			{
				System.out.println("Invalid Server Response Exiting...");
				java.lang.System.exit(0);
			}
			
			//get ID and use it throughout the game
			clientID = splitResponse[1];
			
			System.out.println("role : " + role);
			System.out.println("clientID : " + clientID);
			
			response = this.sendMessage(clientID + " New\r\n");
			
			splitResponse = response.split(" ", 2);
			
			if(splitResponse[0].compareTo("Valid") != 0)
			{
				System.out.println("Server Response Invalid Exiting...");
				java.lang.System.exit(0);
			}
			
			splitResponse = splitResponse[1].split(" ", 26);
			
			System.out.println(splitResponse[24]);
			
			int [] nums = new int[25];
			for(int i = 0; i < 25; i ++)
			{
				nums[i] = Integer.parseInt(splitResponse[i]); 
			}
			
			//make GUI for players
			ClientFrame clientFrame = new ClientFrame(nums, role);
			
			if(role == 0) // Main Culprit
				clientFrame.appendText("You are the Main Culprit\n");
			else // Co-partner
				clientFrame.appendText("You are the Co-partner\n");
			
			while(true) {
				//constantly checking the current game status
				response = this.sendMessage(clientID + " Try\r\n");
				splitResponse = response.split(" ", 0);
				
				//have to wait
				if(splitResponse[0].compareTo("Wait") == 0) {
					Thread.sleep(2000);
					continue;
				}
				//can play
				else if(splitResponse[0].compareTo("Play") == 0) {
					Thread.sleep(2000);
					clientFrame.appendText("Your turn\n");
					clientFrame.setTurn(true);
					
					//get previous values that are taken by other players
					for(int i = 1; i < splitResponse.length; i++) {
						//if there is message
						if(i == 1) {
							if(splitResponse[1].compareTo("Message") == 0) {

								boolean contains = false;
								int temp = Integer.parseInt(splitResponse[2]); 
								int j;
								
								for(j=0;j<25;j++) {
									if(nums[j]==temp) {
										contains = true;
										break;
									}
								}
								if(contains && clientFrame.buttons[j].isVisible())
									clientFrame.setSecretVal(temp);
								else
									clientFrame.appendText("You can ignore the secret value this time : " + temp + "\n");
								i++;
								continue;
							}
						}
						int value = Integer.parseInt(splitResponse[i]);
						clientFrame.setGot(value);
					}
					//wait till the player picks a vlue
					while(true) {
						Thread.sleep(2000);
						if(!clientFrame.getTurn())
							break;
					}
					System.out.println("role : " + role);
					if(role == 0) {
						this.sendMessage(clientID + " Message " + clientFrame.getSecretVal() + "\r\n");
						clientFrame.setSecretVal(0);
						Thread.sleep(1000);
					}
					this.sendMessage(clientID + " Play " + clientFrame.getNextVal() + "\r\n");
				}
				//game ended
				else {
					if(splitResponse[0].compareTo("Finished") == 0) {
						int [] winner = new int[5];
						for(int i=1;i<splitResponse.length;i++) {
							//even if it's finished there can be some values that are picked but not applied to this client
							if(splitResponse[i].compareTo("Prev") != 0)
								winner[Integer.parseInt(splitResponse[i])] = 1;
							else {
								while(++i<splitResponse.length) {
									int value = Integer.parseInt(splitResponse[i]);
									clientFrame.setGot(value);
								}
							}
						}
						System.out.println("Game Finished");
						if(winner[role] == 1)
							clientFrame.appendText("You are the winner!\nCongratulations!\n");
						else
							clientFrame.appendText("Lost\n");
					}
					else {
						System.out.println("Server Response Invalid Exiting...");
					}
					break;
				}
			}
			stopConnection();
		} catch (IOException e) {
			e.printStackTrace();
		} catch (InterruptedException e) {
			e.printStackTrace();
		}
	}
	
}
