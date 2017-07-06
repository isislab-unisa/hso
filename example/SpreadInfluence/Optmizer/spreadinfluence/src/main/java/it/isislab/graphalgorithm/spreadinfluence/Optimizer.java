package it.isislab.graphalgorithm.spreadinfluence;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.Random;
import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.Writer;
import java.util.List;
import java.util.StringTokenizer;

import org.jgrapht.DirectedGraph;
import org.jgrapht.Graph;
import org.jgrapht.UndirectedGraph;
import org.jgrapht.graph.DefaultDirectedGraph;
import org.jgrapht.graph.DefaultEdge;
import org.jgrapht.graph.Pseudograph;
import org.jgrapht.graph.SimpleDirectedGraph;
import org.jgrapht.graph.SimpleGraph;
import org.zeromq.*;
import org.zeromq.ZMQ.Socket;

public class Optimizer {

	private static ZMQ.Context context;
	private static Socket eval, out;

	/**
	 * This class represent simulation
	 */
	private static class SimulationDesign {
		private final String path;
		private final String conf;
		private final int T;  
		private final int num_rep; 

		/**
		 * vector of simulation results
		 */
		public final ArrayList<Integer> ys;

		public double getMeanY() {		
			if (ys.size()==0) return 0.0;
			double ys_sum=0;
			for (double y:ys) ys_sum+=y;
			return ys_sum/ys.size();
		}

		public double getVarianceY() {
			if (ys.size()<=1) return 1.0;
			double mean = getMeanY();
			double temp = 0;
			for(double y:ys)
				temp += (y-mean)*(y-mean);
			return temp/(ys.size()-1);
		}
		public SimulationDesign (String path,String conf,int T, int num_rep) {
			this.path=path;
			this.conf=conf;
			this.T=T;
			this.num_rep=num_rep;
			ys = new ArrayList<Integer>();

		}

		public void add_result(int res){
			ys.add(res);
		}

		public int run_simulation_y() {
			return 0;
		}

		public int getNoSimulations () {
			return ys.size();
		}
	}

	public static void main(String args[]) throws Exception {

		String path_eval, path_out, cpu_model, gpu_model,n_acc;


		/** zeroMQ **/
		context = ZMQ.context(1);

		path_eval = "tcp://"; //change to EVAL address
		path_out = "tcp://"; //change to OUT address

		eval = context.socket(ZMQ.PAIR);
		eval.connect(path_eval);
		out = context.socket(ZMQ.PAIR);
		out.bind(path_out);

		/**send to the master the information about out socket**/
		eval.send(String.valueOf("INIT_OUT::out_address::".length()),0); //change out_address to OUT address
		eval.send("INIT_OUT::aut_address::",0);//change out_address to OUT address


		long startTime = System.currentTimeMillis();
		/**send to the master the information about the simulation models**/
		cpu_model="INIT_CPU::cpu_script.sh::"; //change with path to simulation
		gpu_model="INIT_gpu::gpu_script.sh::";//change with path to simulation
		n_acc = "INIT_NUM::numcpu::numgpu::"; //change numcpu and numgpu

		eval.send(String.valueOf(n_acc.length()),0);
		eval.send(n_acc,0);
		eval.send(String.valueOf(cpu_model.length()),0);
		eval.send(cpu_model,0);
		eval.send(String.valueOf(gpu_model.length()),0);
		eval.send(gpu_model,0);



		/** import graph from TSV file**/
		int num_vertex=0,num_edge=0;

		/** create Gpraph from .tsv file**/
		String graph_filepaht = ""; //change with local path to the graph file
		String cluster_graph_filepath = ""; //change with path to the graph file on the cluster
		System.out.println("Reading file: "+graph_filepaht);
		Graph<String, DefaultEdge> graph = new Pseudograph<String, DefaultEdge>(DefaultEdge.class);

		BufferedReader TSVFile = new BufferedReader(new FileReader(graph_filepaht));
		String dataRow = TSVFile.readLine(); // Read first line.
		int count=0;
		while (dataRow != null){
			count++;
			String s[] = dataRow.split("\\t");
			String v1=s[0];
			String v2=s[1];
			graph.addVertex(v1);
			graph.addVertex(v2);
			graph.addEdge(v1, v2);
			dataRow = TSVFile.readLine(); 
		}	
		TSVFile.close();
		//System.out.println("count "+count);
		num_vertex =graph.vertexSet().size();
		num_edge = graph.edgeSet().size();

		int sum=0;
		int max =0;

		for(String x : graph.vertexSet()){
			sum+=graph.edgesOf(x).size();
		}


		/** Create configuration**/
		int num_conf=200;
		String[] configuration = new String[num_conf]; 
		Random r = new Random(System.currentTimeMillis());

		int S = (sum*5  )/100;
		int T = (sum*50)/100;
		System.out.println("S:"+S);
		System.out.println("T:"+T);


		for (int i=0;i<num_conf;i++){
			int[] c = new int[num_vertex];
			for(int j=0;j<num_vertex;j++){
				c[j]=0;
			}
			for(int j=0;j<S;j++){
				int tmp = r.nextInt(num_vertex)+0;
				c[tmp]++;
			}
			StringBuilder sb = new StringBuilder("");
			for(int j=0;j<num_vertex;j++){
				sb.append(String.valueOf(c[j]));
				if (j!= (num_vertex-1)){
					sb.append(",");
				}
			}
			configuration[i] = sb.toString();

			
		}

		SimulationDesign simulationDesigns[] = new SimulationDesign[num_conf];
		for(int i=0;i<num_conf;i++){
			simulationDesigns[i] = new SimulationDesign(graph_filepaht, configuration[i], T, 15);
		}
		//System.out.println( simulationDesigns.length);
		int N = simulationDesigns.length;

		// initial simulation allocation - run each design 5 times
		StringBuilder sb = new StringBuilder("");
		sb.append("::INPUT::");
		int n_sim=0;
		for (int i=0;i<N;i++) {	
			for(int step=0;step<simulationDesigns[i].num_rep;step++){
				sb.append(cluster_graph_filepath);
				sb.append("_");
				sb.append(configuration[i]);
				sb.append("_");
				sb.append(simulationDesigns[i].T);
				n_sim++;
				sb.append(";");
			}
		}
		sb.append("::"+n_sim+"::");
		String params = sb.toString();
		

		eval.send(String.valueOf(params.length()),0);
		eval.send(params,0);

		out.recv(0);
		byte[] res = out.recv(0);
		String result = new String(res);
		String[] tmp = result.split("::");
		String[] array_res = tmp[1].split(";");
		int num = 0;
		for(int i=0;i<N;i++){
			for(int j=0;j<simulationDesigns[i].num_rep;j++){
				simulationDesigns[i].add_result(Integer.parseInt(array_res[num]));
				num++;
			}
		}

		System.out.println("Results of initial simulation runs");
		for (int i=0;i<N;i++) {
			System.out.print("i="+i);
			for (double y:simulationDesigns[i].ys) 
				System.out.print(" "+y);

			System.out.println();

		}

		for (int step=0;step<100;step++) {
			double mean[] = new double[N];
			double var[] = new double[N];

			int replications[] = new int[N];
			for (int i=0;i<N;i++) {
				mean[i] = simulationDesigns[i].getMeanY();
				var[i]  = simulationDesigns[i].getVarianceY();				
				replications[i]=simulationDesigns[i].getNoSimulations();
			}
			int new_simulations_to_be_allocated = 5; // we allocate in each procedure step five simulations (this will depend on available number of workers in EMR cluster)
			int an[] = OCBAImplementation.ocba(mean, var, replications, new_simulations_to_be_allocated);
			System.out.println("Step "+step+" allocation ");

			for (int i=0;i<N;i++) {	
				System.out.println("i="+i+" runs="+an[i]+" mean="+mean[i]+" std="+Math.sqrt(var[i]));
			}

			sb = new StringBuilder("");
			sb.append("::INPUT::");
			n_sim=0;
			for (int i=0;i<N;i++) {	
				for(int instep=0;instep<an[i];instep++){
					sb.append(cluster_graph_filepath);
					sb.append("_");
					sb.append(configuration[i]);
					sb.append("_");
					sb.append(simulationDesigns[i].T);
					sb.append(";");
					n_sim++;
				}
			}
			sb.append("::"+n_sim+"::");
			params = sb.toString();

			eval.send(String.valueOf(params.length()),0);
			eval.send(params,0);
			out.recv(0);
			res = out.recv(0);
			result = new String(res);
			tmp = result.split("::");
			array_res = tmp[1].split(";");
			num =0;
			for(int i=0;i<N;i++){
				for(int j=0;j<an[i];j++){
					simulationDesigns[i].add_result(Integer.parseInt(array_res[num]));
					num++;
				}
			}


		}

		eval.close();
		out.close();
		context.term();


		System.exit(0);
	}

}
