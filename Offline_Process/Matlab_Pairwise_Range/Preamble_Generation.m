close all
clear 
clc
fs=44100; %48000

Ns = 1920; % lenght of the OFDM
N_FSK = 2720; % length of the FSK
BW = [1000, 5000]; %[1000, 6000];

PN_seq = [1, 1, -1, 1];
L = length(PN_seq);
N_pre = 2;
type_sig = 1;

f_seq = linspace(0,fs,Ns);
mu = 10;
M_user = 14;


%% generate the OFDM symbol
y_fft = complex(zeros(Ns,1));
delta_f = fs / Ns;
begin_i = round(BW(1)/delta_f) + 1;
end_i = round(BW(2)/delta_f);
training_num = end_i - begin_i +1;
zc_seq = cazac_creat2(training_num, 1);
delta_f2 =  fs / N_FSK;
begin_i2 = round(BW(1)/delta_f2) + 1;
end_i2 = round(BW(2)/delta_f2);
training_num2 = end_i2 - begin_i2 +1;
FSK_begin = begin_i2 + 8; 
FSK_end = end_i2 - 14;

for i = begin_i:end_i
    y_fft(i) = zc_seq(i - begin_i+1);
end
train_sig1=real(ifft(y_fft,Ns)*Ns)/training_num;

%% generate the FSK symbol
fsk_idx = [];
y_fsk = complex(zeros(N_FSK,1));
zc_seq2 = cazac_creat2(FSK_end-FSK_begin+1, 1);
for i = FSK_begin:FSK_end
    if mod(i - FSK_begin, M_user) == mu 
        fsk_idx = [fsk_idx, i];
        y_fsk(i) = zc_seq2(i - FSK_begin+1);
    end
end

a = " ";
for ix= fsk_idx
    a = strcat(a, ', ', int2str(ix));
end

train_sig_pad = real(ifft(y_fsk, N_FSK)*N_FSK)/length(fsk_idx);


%% process and CSI estimation
max(abs(train_sig_pad))
train_sig1 = train_sig1/max(abs(train_sig1));
train_sig_pad = train_sig_pad/max(abs(train_sig_pad));
figure
plot(train_sig_pad)
figure
plot(train_sig1)


train_sig2 = [];
GI = 540; % total guard interval
CP = 270; % cyclic prefix
ZS = GI - CP; % zero padding
for i = 1:L
    prefix = train_sig1(end -CP + 1:end);
    train_sig2 = [train_sig2; zeros(ZS, 1); PN_seq(i)*prefix; PN_seq(i)*train_sig1];
end
train_sig2 = [train_sig2; zeros(GI, 1); train_sig_pad];


%%% pad the FSK symbol
rx_signal3 = [zeros(18000, 1); train_sig2; zeros(18000, 1)];
[naiser_idx, peak, Mn] = naiser_corr3(rx_signal3, Ns, GI, L, PN_seq);
figure
subplot(211)
plot(rx_signal3)
subplot(212)
hold on
plot(Mn)
ylim([-0.5,1])


figure
hold on
time_corr = xcorr(train_sig2);
plot(time_corr);


sending_signals = [zeros(24000,1); train_sig2; zeros(24000, 1)]*30000;
max(sending_signals);
min(sending_signals);

save_file = strcat('./multi_users_FSK3/N_seq5-', int2str(mu), '-', int2str(Ns), '-', int2str(GI), '-', int2str(BW(1)), '-', int2str(BW(2)));
preamble = int16(train_sig2*31000);


if (exist(save_file, 'dir') == 0), mkdir(save_file); end

dlmwrite(strcat(save_file,'/FSK_IDX.txt'),fsk_idx,'\n');
dlmwrite(strcat(save_file,'/train_sig1.txt'),train_sig1,'\n');
dlmwrite(strcat(save_file,'/train_pad.txt'),train_sig_pad,'\n');
dlmwrite(strcat(save_file,'/train_sig2.txt'),train_sig2,'\n');

filename = strcat('seq5_fsk_', int2str(mu), '_', int2str(Ns), '_', int2str(GI), '_', int2str(N_FSK), '_', int2str(BW(1)), '_', int2str(N_FSK));
dlmwrite(strcat(save_file,'/', filename ,'.txt'),train_sig1,'\n');
fid=fopen(strcat(save_file,'/', filename, '.bin'),'w+');
fwrite(fid,preamble,'int16');
fclose(fid);
pre2 = zeros(length(preamble), 1);
audiowrite(strcat(save_file, '/music.wav') , [preamble, preamble], fs, 'BitsPerSample',16);
figure
plot(preamble)

