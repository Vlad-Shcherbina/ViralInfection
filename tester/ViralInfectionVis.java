import java.awt.*;
import java.awt.geom.*;
import java.awt.event.*;
import java.awt.image.*;
import java.io.*;
import java.util.*;
import java.security.*;
import javax.swing.*;
import javax.imageio.*;
 
public class ViralInfectionVis {

private int MAX_TIME = 10000;
private String errorMessage = null;

Random r;

int[][] virus;
double[][] med;
int medStrength;
int killTime;
double spreadProb;

int medCount = 0;
int timeCount = 0;

int width, height;

// ----------------------------------------------------------------------------------
private String[] getStatus() {
  String[] ret = new String[virus.length];
  for (int i = 0; i < virus.length; i++) {
    ret[i] = "";
    for (int j = 0; j < virus[i].length; j++)
      ret[i] += virus[i][j] > 0 ? 'V' : (virus[i][j] < 0 ? 'X' : 'C');
  }
  return ret;
}
// ----------------------------------------------------------------------------------
private void generateTestCase(long seed) {
  r = new Random(seed);
  medStrength = r.nextInt(91) + 10;
  killTime = r.nextInt(10) + 1;
  spreadProb = r.nextDouble() * 0.75 + 0.25;
  height = r.nextInt(86) + 15;
  width = r.nextInt(86) + 15;
  if (seed < 5) {
    width = height = 10 + 5 * (int)seed;
  }
  virus = new int[height][width];
  med = new double[height][width];
  int numVirus = killTime + r.nextInt(height * width / 10);
  int numDead = r.nextInt(height * width / 10);
  while (numDead > 0) {
    int y = r.nextInt(height);
    int x = r.nextInt(width);
    if (virus[y][x] != 0) continue;
    virus[y][x] = -1;
    numDead--;
  }
  while (numVirus > 0) {
    int y = r.nextInt(height);
    int x = r.nextInt(width);
    if (virus[y][x] != 0) continue;
    virus[y][x] = killTime;
    numVirus--;
  }  
  r = new Random(seed ^ 987654321987654321L);
  if (debug) {
    String[] t = getStatus();
    String ret = "Med Strength = " + medStrength;
    ret += "\nKill Time = " + killTime + "\nSpread Prob = " + spreadProb + "\n\n";
    for (int i = 0; i < t.length; i++) ret += t[i] + "\n";
    System.out.println(ret);
  }
}
// ----------------------------------------------------------------------------------
public int addMed(int x, int y) {
  if (x < 0 || y < 0 || x >= med[0].length || y >= med.length) {
    errorMessage = "addMed() called with invalid parameters";
    return -1;
  }
  if (timeCount >= MAX_TIME) {
    errorMessage = "addMed() called after max time exceeded.";
    return -1;
  }
  med[y][x] += medStrength;
  medCount++;
  incrementTime();
  return 0;
}
// ----------------------------------------------------------------------------------
public String[] observe() {
  String[] ret = getStatus();
  incrementTime();
  return ret;
}
// ----------------------------------------------------------------------------------
public int waitTime(int units) {
  if (units < 1 || timeCount + units > MAX_TIME) {
    errorMessage = "waitTime() called with invalid parameters";
    return -1;
  }
  for (int i = 0; i < units && timeCount < MAX_TIME; i++) incrementTime();
  return 0;
}
// ----------------------------------------------------------------------------------
private void incrementTime() {
  processMeds();
  processViruses();
  diffuse();
  timeCount++;
  if (debug) {
    System.out.println("Elapsed time: " + timeCount);
  }
  draw();
}
// ----------------------------------------------------------------------------------
private void processMeds() {
  for (int y = 0; y < virus.length; y++)
    for (int x = 0; x < virus[y].length; x++)
      if (virus[y][x] > 0 && med[y][x] >= 1.0)
        virus[y][x] = 0;
}
// ----------------------------------------------------------------------------------
private void processViruses() {
  for (int y = 0; y < virus.length; y++)
    for (int x = 0; x < virus[y].length; x++) {
	  if (virus[y][x] <= 0) continue;
	  virus[y][x]--;
	  if (virus[y][x] == 0) {
	    virus[y][x] = -2;
	  }
	}
  for (int y = 0; y < virus.length; y++)
    for (int x = 0; x < virus[y].length; x++) {
	  if (virus[y][x] == -2) {
	    virus[y][x] = -1;
	    spreadVirus(x, y);
	  }
	}
}
// ----------------------------------------------------------------------------------
private void spreadVirus(int x, int y) {
  if (x > 0 && virus[y][x - 1] == 0 && r.nextDouble() < spreadProb) virus[y][x - 1] = killTime;
  if (x < virus[y].length - 1 && virus[y][x + 1] == 0 && r.nextDouble() < spreadProb) virus[y][x + 1] = killTime;
  if (y > 0 && virus[y - 1][x] == 0 && r.nextDouble() < spreadProb) virus[y - 1][x] = killTime;
  if (y < virus.length- 1 && virus[y + 1][x] == 0 && r.nextDouble() < spreadProb) virus[y + 1][x] = killTime;
}
// ----------------------------------------------------------------------------------
private void diffuse() {
  double[][] diff = new double[med.length][med[0].length];
  for (int y = 0; y < med.length; y++)
    for (int x = 0; x < med[y].length; x++) {
      if (x > 0) {
        diff[y][x - 1] += (med[y][x] - med[y][x - 1]) * 0.2;
        diff[y][x] += (med[y][x - 1] - med[y][x]) * 0.2;
      }
      if (y > 0) {
        diff[y - 1][x] += (med[y][x] - med[y - 1][x]) * 0.2;
        diff[y][x] += (med[y - 1][x] - med[y][x]) * 0.2;
      }
    }
  for (int y = 0; y < med.length; y++)
    for (int x = 0; x < med[y].length; x++)
      med[y][x] += diff[y][x];
}
// ----------------------------------------------------------------------------------
private boolean doneSim() {
  if (timeCount >= MAX_TIME) return true;
  for (int y = 0; y < med.length; y++)
    for (int x = 0; x < med[y].length; x++)
      if (virus[y][x] > 0)
        return false;
  return true;
}
// ----------------------------------------------------------------------------------
public double runTest(String test) {
  try {
  long seed = Long.parseLong(test);
  generateTestCase(seed);
  if (vis) {
    jf.setSize(width*scale+50,height*scale+40);
    jf.setVisible(true);
    draw();
  }

  int ret = runSim(getStatus(), medStrength, killTime, spreadProb);
  // visualizer sets return from runSim to 0 if it ran successfully, and -1 otherwise
  if (ret == -1) {
    addFatalError("One or more issued queries contained invalid parameters.");
    return 0;
  }

  while (!doneSim()) 
    incrementTime();
  int healthy = 0;
  for (int y = 0; y < med.length; y++)
    for (int x = 0; x < med[y].length; x++)
      if (virus[y][x] == 0) 
        healthy++;
  if (debug) {
    System.out.println("After " + timeCount + " time:");
    String[] s = getStatus();
    for (int i = 0; i < s.length; i++) 
      System.out.println(s[i]);
  }
  return Math.max(1.0 * (healthy - medCount * 0.5) / Math.max(timeCount, 1), 0.0);
  }
  catch (Exception e) {
    addFatalError("RunTest Exception: " + e.getMessage() + "\n");
    StringWriter sw = new StringWriter();
    PrintWriter pw = new PrintWriter(sw);
    e.printStackTrace(pw);
    addFatalError(sw.toString() + "\n");
    return -1;
  }
}

// ------------- server part --------------------------------------------------------
public String checkData(String s) { return ""; }
// ----------------------------------------------------------------------------------
public String displayTestCase(String s) {
  long seed = Long.parseLong(s);
  generateTestCase(seed);
  String[] t = getStatus();
  String ret = "Med Strength = " + medStrength;
  ret += "\nKill Time = " + killTime + "\nSpread Prob = " + spreadProb + "\n\n";
  for (int i = 0; i < t.length; i++) ret += t[i] + "\n";
  return ret;
}
// ----------------------------------------------------------------------------------
public double[] score(double[][] raw) {
  double[] ret = new double[raw.length];
  double[] best = new double[raw[0].length];
  for (int i = 0; i < raw.length; i++)
    for (int j = 0; j < raw[i].length; j++)
      best[j] = Math.max(best[j], raw[i][j]);
  for (int i = 0; i < raw.length; i++)
    for (int j = 0; j < raw[i].length; j++)
      if (raw[i][j] > 0 && best[j] > 0)
        ret[i] += raw[i][j] / best[j];
  for (int i = 0; i < raw.length; i++) {
    ret[i] /= best.length;
    ret[i] *= 1000000.0;
  }
  return ret;
}
// ------------- visualization part -----------------------------------------------------
    static int scale;
    static String exec;
    static boolean debug, vis;
    static Process proc;
    static int del;
    InputStream is;
    OutputStream os;
    BufferedReader br;
    JFrame jf;
    Vis v;
    // ----------------------------------------------------------------------------------
    int runSim(String[] slide, int medStrength, int killTime, double spreadProb) {
      try {
        if (exec != null)
        {   // imitate passing params
            StringBuffer sb = new StringBuffer();
            sb.append(height).append('\n');
            for (int i = 0; i < height; ++i)
                sb.append(slide[i]).append('\n');
            sb.append(medStrength).append('\n');
            sb.append(killTime).append('\n');
            sb.append(spreadProb).append('\n');
            os.write(sb.toString().getBytes());
            os.flush();

            // imitate method calls - they start with method name in separate line followed by params if required
            String s;
            while ((s = br.readLine()) != null && !s.equals("END")) {
                String[] callRes;
                if (s.equals("ADDMED")) {
                    // followed by "x y"
                    String[] params = br.readLine().split(" ");
                    if (params.length != 2) {
                        // invalid # of arg
                        return -1;
                    }
                    int x, y;
                    try {
                        x = Integer.parseInt(params[0]);
                        y = Integer.parseInt(params[1]);
                    } catch (NumberFormatException e) {
                        // failed to convert query arg to ints
                        return -1;
                    }
                    if (debug) {
                        System.out.println("Add med at (" + x + "," + y + ")");
                    }
                    if (addMed(x, y) == -1) {
                        addFatalError("Error in addMed() call: " + errorMessage);
                        return -1;
                    }
                    callRes = new String[0];
                } else
                if (s.equals("OBSERVE")) {
                    if (debug) {
                        System.out.println("Observe");
                    }
                    callRes = observe();
                } else
                if (s.equals("WAITTIME")) {
                    String param = br.readLine();
                    int time;
                    try {
                        time = Integer.parseInt(param);
                    } catch (NumberFormatException e) {
                        // failed to convert query arg to ints
                        return -1;
                    }
                    if (debug) {
                        System.out.println("Wait for " + time);
                    }
                    if (waitTime(time) == -1) {
                        addFatalError("Error in waitTime() call: " + errorMessage);
                        return -1;
                    }
                    callRes = new String[0];
                } else {
                    // unknown command
                    addFatalError("Unknown command '" + s + "'.");
                    return -1;
                }

                // and return call result to the solution
                String ret = "";
                ret += callRes.length + "\n";
                for (int i = 0; i < callRes.length; ++i) {
                    ret += callRes[i] + "\n";
                }
                os.write(ret.getBytes());
                os.flush();

                // drawing happens inside incrementTime
            }
            addFatalError("Finished processing solution input.");
            // no need to parse return from runSim
        }
        return 0;
      } catch (IOException e) {
        return -1;
      }
    }
    // ----------------------------------------------------------------------------------
    void draw() {
        if (!vis) return;
        v.repaint();
        try { Thread.sleep(del); }
        catch (Exception e) { };
    }
    // ----------------------------------------------------------------------------------
    public class Vis extends JPanel implements WindowListener {
        public void paint(Graphics gr) {
            int i, j;
            BufferedImage bi = new BufferedImage(width * scale + 150, height * scale + 1, BufferedImage.TYPE_INT_RGB);
            Graphics2D g2 = (Graphics2D)bi.getGraphics();
            // background
            g2.setColor(new Color(0xEEEEEE));
            g2.fillRect(0, 0, width * scale + 150, height * scale + 1);
            g2.setColor(new Color(0xAAAAAA));
            g2.fillRect(0, 0, width * scale, height * scale);
            // cell color marks status: blue for healthy (virus = 0), gray for dead (virus < 0), red for infected (virus > 0)
            // color intensity marks medicine concentration: lighter shade = less medicine
            // virus[i][j] > 0 ? 'V' : (virus[i][j] < 0 ? 'X' : 'C');
            for (i = 0; i < height; ++i)
            for (j = 0; j < width; ++j) {
                int mult = (int)(200. * (1. - med[i][j] / medStrength));
                int c;
                if (virus[i][j] < 0) {
                    // dead
                    c = 0x010101 * mult;
                } else if (virus[i][j] == 0) {
                    // healthy
                    c = 0x0000FF + 0x010100 * mult;
                    //c = 0x000001 * mult;
                } else {
                    // infected
                    c = 0xFF0000 + 0x000101 * mult;
                    //c = 0x010000 * mult;
                }
                g2.setColor(new Color(c));
                g2.fillRect(j * scale + 1, i * scale + 1, scale, scale);
            }

            gr.drawImage(bi, 0, 0, width * scale + 150, height * scale + 1, null);
//            try { ImageIO.write(bi, "png", new File("1.png")); } catch (Exception e) {};
        }
        public Vis() {
            jf.addWindowListener(this);
        }
        //WindowListener
        public void windowClosing(WindowEvent e){ 
            if(proc != null)
                try { proc.destroy(); } 
                catch (Exception ex) { ex.printStackTrace(); }
            System.exit(0); 
        }
        public void windowActivated(WindowEvent e) { }
        public void windowDeactivated(WindowEvent e) { }
        public void windowOpened(WindowEvent e) { }
        public void windowClosed(WindowEvent e) { }
        public void windowIconified(WindowEvent e) { }
        public void windowDeiconified(WindowEvent e) { }
    }
    // ----------------------------------------------------------------------------------
    public ViralInfectionVis(String seed) {
      try {
        // interface for runTest
        if (vis)
        {   jf = new JFrame();
            v = new Vis();
            jf.getContentPane().add(v);
        }
        if (exec != null) {
            try {
                Runtime rt = Runtime.getRuntime();
                proc = rt.exec(exec);
                os = proc.getOutputStream();
                is = proc.getInputStream();
                br = new BufferedReader(new InputStreamReader(is));
                new ErrorReader(proc.getErrorStream()).start();
            } catch (Exception e) { e.printStackTrace(); }
        }
        System.out.println("Score = "+runTest(seed));
        if (proc != null)
            try { proc.destroy(); } 
            catch (Exception e) { e.printStackTrace(); }
      }
      catch (Exception e) { e.printStackTrace(); }
    }
    // ----------------------------------------------------------------------------------
    public static void main(String[] args) {
        String seed = "1";
        vis = true;
        debug = false;
        del = 100;
        scale = 5;
        for (int i = 0; i<args.length; i++)
        {   if (args[i].equals("-seed"))
                seed = args[++i];
            if (args[i].equals("-exec"))
                exec = args[++i];
            if (args[i].equals("-novis"))
                vis = false;
            if (args[i].equals("-debug"))
                debug = true;
            if (args[i].equals("-delay"))
                del = Integer.parseInt(args[++i]);
            if (args[i].equals("-scale"))
                scale = Integer.parseInt(args[++i]);
        }
        ViralInfectionVis bw = new ViralInfectionVis(seed);
 
    }
    // ----------------------------------------------------------------------------------
    void addFatalError(String message) {
        System.out.println(message);
    }
}
 
class ErrorReader extends Thread{
    InputStream error;
    public ErrorReader(InputStream is) {
        error = is;
    }
    public void run() {
        try {
            byte[] ch = new byte[50000];
            int read;
            while ((read = error.read(ch)) > 0)
            {   String s = new String(ch,0,read);
                System.out.print(s);
                System.out.flush();
            }
        } catch(Exception e) { }
    }
}
