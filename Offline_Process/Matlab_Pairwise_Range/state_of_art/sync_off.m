close all
clear 
% clc
fs = 44100;

BW = [1000, 5000]; %[1000, 6000];
B = BW(2) - BW(1);

save_file = strcat('../FMCW/');
train_sig = dlmread(strcat(save_file,'/train_sig.txt'));
Ns = length(train_sig);
f_seq = linspace(0, fs-fs/Ns, Ns);
file_id = '7';


numstr = '1675994759556';
raw_folder = strcat('../../Range_data/sync/');
rx_signal = dlmread(strcat(raw_folder, '/', numstr, '/', numstr, '-',file_id,'-bottom.txt'))/10000;
rx_signal = rx_signal(3*fs:end);
filter_order = 256;
wn = [(BW(1) - 100)/(fs/2), (BW(2) + 200)/(fs/2)];    
b = fir1(filter_order, wn, 'bandpass');  

y_after_fir=filter(b,1,rx_signal);
delay_fir = filter_order/2;
rx_signal = y_after_fir(delay_fir+1:end);


%% using the cross correlation to detect preamble
[acor, lag] = xcorr(rx_signal, train_sig);
max_peak_corr = max(acor);
[pks,locs]=findpeaks(acor,'MinPeakHeight',max_peak_corr*0.1,'MinPeakDistance',fs*1);

seek_back = 480;
new_locs = [];
for i = 1:length(locs)
    l= locs(i);
    p = pks(i);
    p_threshold = p*0.65; %0.65

    for j = seek_back : -1 : 0
        l_new = l - j;
        if(acor(l_new) > p_threshold)
            new_locs = [new_locs; l_new];
            break;
        end
    end
end

figure
hold on
plot(lag, acor)
scatter(lag(new_locs), acor(new_locs), 'rx')

new_locs = new_locs(6:end);
interval = round(1.5*44100);
relative  = new_locs - new_locs(1);
residual = mod(relative, interval);

% residual(residual > 30000) = residual(residual > 30000) - interval;

T = 50;
x = 0:T;
p = polyfit(x,residual(1:T+1),1);

ts = 1.5*(0:length(residual)-1);


xx = 1:length(residual);
figure
hold on
plot(residual)
plot(xx, p(1)*xx + p(2))

residual = residual' - round(p(1)*(0:length(residual)-1));

figure
plot(ts/60, residual)

dlmwrite(strcat('ts.txt'), ts);
dlmwrite(strcat('residual_60.txt'), residual);

