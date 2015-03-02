a=textread('at.txt');
figure(1)
plot(a(1,:),a(2,:),'-k+');
hold on
plot(a(1,:),a(3,:),'-ko');
hold on
plot(a(1,:),a(4,:),'-k*');
axis([1 10 0 70]);
grid on;
xlabel('Arrival Rate (jobs per second)');
ylabel('Time (second)');
hold off;

a=textread('ut.txt');
figure(2)
plot(a(1,:),a(2,:),'-k+');
hold on
plot(a(1,:),a(3,:),'-ko');
hold on
plot(a(1,:),a(4,:),'-k*');
axis([1 10 0 100]);
grid on;
xlabel('Arrival Rate (jobs per second)');
ylabel('Utilization (percent)');
hold off;

a=textread('an1.txt');
figure(3)
plot(a(1,:),a(2,:),'-k+');
hold on
plot(a(1,:),a(3,:),'-ko');
hold on
plot(a(1,:),a(4,:),'-k*');
axis([1 10 0 600]);
grid on;
xlabel('Arrival Rate (jobs per second)');
ylabel('Number of Jobs');
hold off;

a=textread('an2.txt');
figure(4)
plot(a(1,:),a(2,:),'-k+');
hold on
plot(a(1,:),a(3,:),'-ko');
hold on
plot(a(1,:),a(4,:),'-k*');
axis([1 10 0 3]);
xlabel('Arrival Rate (jobs per second)');
ylabel('Number of Jobs');
grid on;
hold off;