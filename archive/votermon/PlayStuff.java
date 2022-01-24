/*
* VOTER System Monitor Applet
*
* Copyright (C) 2011
* Jim Dixon, WB6NIL <jim@lambdatel.com>
*
* This file is part of the VOTER System Project
*
*   VOTER System is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 2 of the License, or
*   (at your option) any later version.

*   Voter System is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this project.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
 * PlayStuff.java
 */

package votermon;

import java.net.*;
import java.io.IOException;
import javax.sound.sampled.AudioFormat;
import javax.sound.sampled.AudioSystem;
import javax.sound.sampled.DataLine;
import javax.sound.sampled.LineUnavailableException;
import javax.sound.sampled.SourceDataLine;



public class PlayStuff extends Thread {

    private String hostname = null;
    private short port = 0;
    SourceDataLine auline = null;
    private Boolean stopit = false;
    private VoterMon MyView;
    private Boolean running = false;
    private static float xv[] = new float[7];
    private static float yv[] = new float[7];

    public PlayStuff(String myhostname,short myport,VoterMon myview) {
        hostname = myhostname;
        port = myport;
        MyView = myview;
    }

    public void StopPlay() {
        if (auline == null) return;
        auline.stop();
        auline.close();
        stopit = true;
    }

    private final static int exp2[] = {0, 132, 396, 924, 1980,
        4092, 8316, 16764};
    public static int ulawToLinear(byte ulawbyte) {
       int sign, exponent, mantissa, sample;
       ulawbyte = (byte) ~ulawbyte;
       sign = (ulawbyte & 0x80);
       exponent = (ulawbyte >> 4) & 0x07;
       mantissa = ulawbyte & 0x0F;
       sample = exp2[exponent] + (mantissa << (exponent + 3));
       if (sign != 0) sample = -sample;
       return sample;
    }

    @Override
    public void run() {
        Socket skt = null;
        long timesec = 0,timensec = 0;
        int i,j;

        try {
           skt = new Socket(hostname, port);
        } catch (Exception e) {
           MyView.PlayStopped();
           return;
        }

        AudioFormat format = new AudioFormat(AudioFormat.Encoding.PCM_SIGNED,8000,16,1,2,8000,false);

        DataLine.Info info = new DataLine.Info(SourceDataLine.class, format);

        try {
            auline = (SourceDataLine) AudioSystem.getLine(info);
            auline.open(format);
        } catch (LineUnavailableException e) {
            e.printStackTrace();
            return;
        } catch (Exception e) {
            e.printStackTrace();
            return;
        }

        auline.start();
        int nBytesRead = 0;
        long totalBytesRead = 0;
        byte[] buf = new byte[320];
        byte[] outbuf = new byte[320];
        byte[] b = new byte[1];
        String clientstr = "";
        try {
            while (stopit == false){
                 if (skt.getInputStream().read(b) != 1) break;
                 buf[nBytesRead++] = b[0];
                 if (nBytesRead < 320) continue;
                 totalBytesRead += nBytesRead;
                 nBytesRead = 0;

                 if (buf[168] == 0) {
                      auline.drain();
                      auline.stop();
                      running = false;
                      MyView.DoClients();
                      continue;
                  }
                  if (running == false) {
                      auline.start();
                      running = true;
                  }
                  for(i = 0; i < 160; i++)
                  {
                      int lin = ulawToLinear(buf[i + 8]);
                      outbuf[i * 2] = (byte)(lin & 0xff);
                      outbuf[(i * 2) + 1] = (byte)((lin & 0xff00) >> 8);
                  }
                  auline.write((byte[])outbuf, 0, outbuf.length);
                  while(((auline.getBufferSize() - auline.available()) > 128) && (stopit == false)) {
                    yield();
                  }
                  timesec = (long)(buf[0] & 255);
                  timesec += (long)((buf[1] & 255) << 8);
                  timesec += (long)((buf[2] & 255) << 16);
                  timesec += (long)((buf[3] & 255) << 24);
                  timensec = (long)(buf[4] & 255);
                  timensec += (long)((buf[5] & 255) << 8);
                  timensec += (long)((buf[6] & 255) << 16);
                  timensec += (long)((buf[7] & 255) << 24);
                  clientstr = "";
                  for(i = 168; (i < 320) && (buf[i] != 0); i++) {
                      char c = (char)buf[i];
                      clientstr += Character.toString(c);
                  }
                  String clients[] = clientstr.split(",");
                  int nclients = clients.length - 1;
                  if (nclients <= 0) continue;
                  int rssi[] = new int[nclients];
                  String name[] = new String[nclients];
                  for(i = 0; i < nclients; i++)
                  {
                      String foo[] = clients[i + 1].split("=");
                      if (foo.length < 2) continue;
                      name[i] = foo[0];
                      rssi[i] = Integer.parseInt(foo[1]);
                  }
                  MyView.DoClients(nclients,clients[0],name,rssi);
            }
        } catch (IOException e) {
            e.printStackTrace();
            return;
        } finally {
            auline.drain();
            auline.close();
            try {
                skt.close();
            } catch (Exception e1) {
            }
            MyView.PlayStopped();
        }

    }
}
