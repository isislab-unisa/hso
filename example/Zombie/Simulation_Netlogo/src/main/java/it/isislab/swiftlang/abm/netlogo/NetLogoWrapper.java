package it.isislab.swiftlang.abm.netlogo;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.HashMap;

import org.kohsuke.args4j.CmdLineException;
import org.kohsuke.args4j.CmdLineParser;
import org.kohsuke.args4j.Option;
import org.nlogo.api.CompilerException;
import org.nlogo.api.LogoException;
import org.nlogo.headless.HeadlessWorkspace;

import scala.util.Random;
/**
@Author(
		   name = "Carmine Spagnuolo",
		   date = "05/02/2016"
		)
 */
public class NetLogoWrapper 
{

	HeadlessWorkspace workspace;

	@Option(name="-m",usage="netlogo model path")
	private String model_path;

	@Option(name="-outfile",usage="output to this file",metaVar="OUTPUT")
	private File out = new File(".");

	@Option(name="-trial",usage="number of runs")
	private Integer trial;
	
	@Option(name="-runid",usage="run identify")
	private String id;

	@Option(name="-s",usage="number of steps")
	private Integer steps;

	@Option(name="-i",usage="input list: var1,value1,var2,value2")
	private String input;

	@Option(name="-o",usage="output list: var1,value1,var2,value2")
	private String output;

	HashMap<String, Object> outputs=new HashMap<String, Object>();

	private boolean toPrint=true;
	class PrintWait extends Thread{

		@Override
		public void run() {
			String[] phases = {"|", "/", "-", "\\"};

			while (toPrint)
			{
				for (String phase : phases)
				{
					System.out.print(("\r"+phase));
					try {
						Thread.sleep(50);
					} catch (InterruptedException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
				}
			}
			try {
				String[] arguments_output=output.split(",");
				for (int i = 0; i < arguments_output.length; i+=1) {

					Object outvalue=outputs.get(arguments_output[i]);
					if(outvalue==null)
						outputs.put(arguments_output[i], workspace.report(arguments_output[i]));
					else{
						try{
							String sout=(String)outvalue;
							sout+="-"+workspace.report(arguments_output[i]);
							outputs.put(arguments_output[i], sout);
							
						}catch(Exception e1)
						{
							try{
								Integer sout=(Integer)outvalue;
								sout+=(Integer)workspace.report(arguments_output[i]);
								outputs.put(arguments_output[i], sout);
								
							}catch(Exception e2)
							{
								try{
									Double sout=(Double)outvalue;
									sout+=(Double)workspace.report(arguments_output[i]);
									outputs.put(arguments_output[i], sout);
								}catch(Exception e3)
								{
									e3.getStackTrace();
								}
							}
						}
						
					}

				}
			} catch (CompilerException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			} catch (LogoException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}


		}
	}
	public static void main(String[] args) throws IOException {
		new NetLogoWrapper().doMain(args);
	}

	public void doMain(String[] args) throws IOException {

		CmdLineParser parser = new CmdLineParser(this);


		try {
			parser.parseArgument(args);
			if( input.isEmpty() )
				throw new CmdLineException(parser,"No model paramters in input is given");

		} catch( CmdLineException e ) {
			System.err.println(e.getMessage());
			System.err.println("java NetLogoWrapper [options...] arguments...");
			parser.printUsage(System.err);
			System.err.println();

			return;
		}

		System.out.println("NetLogo model: "+model_path);

		System.out.println("Output file: "+ out);

		Random r=new Random(System.currentTimeMillis());

		String[] arguments=input.split(",");
		HashMap<String, String> parameter=new HashMap<String, String>();
		System.out.println("Model parameters:");
		for (int i = 0; i < arguments.length; i+=2) {
			try{
				System.out.println(arguments[i]+" "+arguments[i+1]);

				parameter.put(arguments[i], arguments[i+1]);
			}catch (Exception e) {
				System.out.println("java NetLogoWrapper [options...] arguments...");
				System.out.println("You must pass parameters setting as couple: var_name1 value1 var_name2 vaue2 ...");
				// print the list of available options
				parser.printUsage(System.err);
				System.exit(-1);
			}
		}
		System.out.println("Start simulation: ");
		workspace = HeadlessWorkspace.newInstance() ;
		try {
			workspace.open(model_path);

			
			for (int i = 0; i < trial; i++) {
				toPrint=true;
				int seed=r.nextInt();
				System.out.println("Run "+i+" with seed: "+seed);
				
				for(String variable_name: parameter.keySet())
				{
					workspace.command("set "+variable_name+" "+parameter.get(variable_name));
				}
				workspace.command("random-seed "+seed);
				workspace.command("setup");
				PrintWait waiter= new PrintWait();
				waiter.start();

				workspace.command("repeat "+steps+" [ go ]") ;

				toPrint=false;
				waiter.join();
				System.out.println("End run "+i);

			}
			workspace.dispose();

			System.out.println("\nOutput parameters:");
			PrintWriter print_output;
			try {
				print_output = new PrintWriter(out);
				String[] arguments_output=output.split(",");
				print_output.print("\"run\",\"tick\"");
				for (int i = 0; i < arguments_output.length; i+=1) {
					print_output.print(",\""+arguments_output[i]+"\"");
				}
				print_output.print("\n");
				print_output.print(id+","+steps);
				for (int i = 0; i < arguments_output.length; i+=1) {
					
					if(outputs.get(arguments_output[i]) instanceof String)
					{
						print_output.print(","+outputs.get(arguments_output[i]));
					}
					else{
						if(outputs.get(arguments_output[i]) instanceof Integer)
							{
								System.out.println(arguments_output[i]+" "+((Integer)outputs.get(arguments_output[i])/trial));
								print_output.print(","+((Integer)outputs.get(arguments_output[i])/trial));
							}
						else{
							System.out.println(arguments_output[i]+" "+((Double)outputs.get(arguments_output[i])/trial));
							print_output.print(","+((Double)outputs.get(arguments_output[i])/trial));
						}
							
					}
					
				}
				print_output.print("\n");
				print_output.close();
			} catch (FileNotFoundException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
		catch(Exception ex) {
			ex.printStackTrace();
		}

	}
}